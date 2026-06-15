#include "scoring/ScoreEngine.hpp"
#include "core/EventBus.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include <algorithm>

namespace hypeclip {

ScoreEngine::ScoreEngine() = default;

void ScoreEngine::start() {
    auto& bus = EventBus::instance();
    busToken_ = bus.onContribution([this](const Contribution& c) {
        ingest(c);
        HighlightEvent e; bool fired = false;
        evaluate(c.when, e, fired);
        if (fired) EventBus::instance().publish(e);
    });
}

void ScoreEngine::stop() {
    if (busToken_) { EventBus::instance().unsubscribe(busToken_); busToken_ = 0; }
}

float ScoreEngine::currentConfidence() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return lastConfidence_;
}

void ScoreEngine::ingest(const Contribution& c) {
    std::lock_guard<std::mutex> lk(mtx_);
    window_.push_back(c);
    momentum_.add(c);
}

void ScoreEngine::pruneOldEntries(TimePoint now) {
    while (!window_.empty() && now - window_.front().when > kWindow)
        window_.pop_front();
}

bool ScoreEngine::passesFailsafes(const HighlightEvent& e, TimePoint now) {
    const Settings s = Config::instance().get();

    // 1) Anti-spam: enforce minimum spacing between clips.
    if (lastClipTime_.time_since_epoch().count() != 0 &&
        now - lastClipTime_ < Millis(s.minClipGapMs)) {
        // Allow an override only for a clearly bigger moment (escalation).
        const bool muchBigger = e.confidence > lastConfidence_ + 20.0f &&
                                e.type != lastClipType_;
        if (!muchBigger) return false;
    }
    // 2) Duplicate guard: same type back-to-back within the spacing window.
    if (e.type == lastClipType_ &&
        lastClipTime_.time_since_epoch().count() != 0 &&
        now - lastClipTime_ < Millis(s.minClipGapMs * 2)) {
        return false;
    }
    return true;
}

float ScoreEngine::evaluate(TimePoint now, HighlightEvent& out, bool& fired) {
    fired = false;
    std::lock_guard<std::mutex> lk(mtx_);
    pruneOldEntries(now);
    momentum_.decayTo(now);

    // Aggregate per-layer scores with recency weighting (newer = heavier).
    LayerScores layers;
    HighlightType bestType = HighlightType::Generic;
    float bestTypeScore = 0.0f;
    std::vector<Contribution> contributors;

    for (const auto& c : window_) {
        const float age = std::chrono::duration<float>(now - c.when).count();
        const float recency = std::max(0.2f, 1.0f - age / 4.0f);
        const float s = c.score * recency;
        switch (c.source) {
            case TriggerSource::GameAudio: layers.audio  += s; break;
            case TriggerSource::MicVoice:  layers.voice  += s; break;
            case TriggerSource::Vision:    layers.vision += s; break;
            default: break;
        }
        if (c.score > bestTypeScore) { bestTypeScore = c.score; bestType = c.type; }
        contributors.push_back(c);
    }
    // Layer 4: event history density (how many events recently).
    layers.history = std::min(40.0f, 8.0f * (float)momentum_.recentEventCount());
    layers.momentum = momentum_.momentum();

    const Settings s = Config::instance().get();
    float conf = confidence_.fuse(layers, s.weights);
    lastConfidence_ = conf;

    if (conf < s.threshold) return conf;

    // Build the event.
    out = HighlightEvent{};
    out.confidence  = conf;
    out.rawScore    = layers.audio + layers.voice + layers.vision + layers.history;
    out.type        = momentum_.escalateType(bestType);
    out.triggeredAt = now;
    out.contributors= std::move(contributors);
    out.majorEvent  = conf >= 85.0f ||
                      out.type == HighlightType::Ace ||
                      out.type == HighlightType::TeamWipe ||
                      out.type == HighlightType::MatchWinningPlay ||
                      out.type == HighlightType::Victory;

    if (!passesFailsafes(out, now)) return conf;

    lastClipTime_ = now;
    lastClipType_ = out.type;
    fired = true;
    HC_INFO("Highlight FIRED: %s @ %.0f%% (audio=%.0f voice=%.0f vision=%.0f mom=%.2f)",
            to_string(out.type), conf, layers.audio, layers.voice, layers.vision, layers.momentum);
    return conf;
}

} // namespace hypeclip
