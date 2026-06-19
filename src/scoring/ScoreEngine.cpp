#include "scoring/ScoreEngine.hpp"
#include "core/EventBus.hpp"
#include "core/Config.hpp"
#include "core/MetricsHub.hpp"
#include "core/Log.hpp"
#include <algorithm>

namespace hypeclip {

ScoreEngine::ScoreEngine() = default;

void ScoreEngine::start() {
    reloadConfig();
    busToken_ = EventBus::instance().onContribution([this](const Contribution& c) {
        ingest(c);
        HighlightEvent e;
        if (evaluate(c.when, e)) EventBus::instance().publish(e);
    });
}
void ScoreEngine::stop() { if (busToken_) { EventBus::instance().unsubscribe(busToken_); busToken_ = 0; } }

void ScoreEngine::reloadConfig() {
    std::lock_guard<std::mutex> lk(mtx_);
    rules_.setRules(Config::instance().get().rules);
}

float ScoreEngine::currentConfidence() const { std::lock_guard<std::mutex> lk(mtx_); return lastConfidence_; }

void ScoreEngine::ingest(const Contribution& c) {
    std::lock_guard<std::mutex> lk(mtx_);
    window_.push_back(c);
    momentum_.add(c);
}
void ScoreEngine::pruneOldEntries(TimePoint now) {
    while (!window_.empty() && now - window_.front().when > kWindow) window_.pop_front();
}

bool ScoreEngine::passesFailsafes(const HighlightEvent& e, TimePoint now) {
    const Settings s = Config::instance().get();
    if (lastClipTime_.time_since_epoch().count() != 0 &&
        now - lastClipTime_ < Millis(s.minClipGapMs)) {
        const bool muchBigger = e.confidence > lastConfidence_ + 20.0f && e.type != lastClipType_;
        if (!muchBigger) return false;
    }
    if (e.type == lastClipType_ && lastClipTime_.time_since_epoch().count() != 0 &&
        now - lastClipTime_ < Millis(s.minClipGapMs * 2)) return false;
    return true;
}

bool ScoreEngine::evaluate(TimePoint now, HighlightEvent& out) {
    std::lock_guard<std::mutex> lk(mtx_);
    const Settings s = Config::instance().get();
    if (!s.masterEnabled) return false;

    pruneOldEntries(now);
    momentum_.decayTo(now);

    // Aggregate recency-weighted per-layer scores from the window.
    LayerScores layers;
    HighlightType bestType = HighlightType::Generic; float bestTypeScore = 0;
    std::vector<Contribution> contributors;
    for (const auto& c : window_) {
        const float age = std::chrono::duration<float>(now - c.when).count();
        const float recency = std::max(0.2f, 1.0f - age / 4.0f);
        const float sc = c.score * recency;
        switch (c.source) {
            case TriggerSource::GameAudio: layers.audio  += sc; break;
            case TriggerSource::MicVoice:  layers.voice  += sc; break;
            case TriggerSource::Vision:    layers.vision += sc; break;
            default: break;
        }
        if (c.score > bestTypeScore) { bestTypeScore = c.score; bestType = c.type; }
        contributors.push_back(c);
    }
    layers.history  = std::min(40.0f, 8.0f * (float)momentum_.recentEventCount());
    layers.momentum = momentum_.momentum();

    const float confidence = s.features.aiScoring ? confidence_.fuse(layers, s.weights) : 0.0f;
    lastConfidence_ = confidence;

    // Build the live snapshot (mic/game/vision come from MetricsHub).
    MetricSnapshot snap = MetricsHub::instance().get();
    snap.momentum   = layers.momentum * 100.0f;
    snap.confidence = confidence;
    learner_.observe(snap, now);

    const float ruleProgress = rules_.bestProgress(snap);
    const float clipWorthiness = std::max(confidence, ruleProgress);
    snap.clipWorthiness = clipWorthiness;
    MetricsHub::instance().updateScoring(snap.momentum, confidence, clipWorthiness);

    // Track how long we have been "hot" (for the learner's sustained check).
    if (clipWorthiness >= 50.0f) {
        if (aboveSince_.time_since_epoch().count() == 0) aboveSince_ = now;
    } else aboveSince_ = TimePoint{};
    const int sustainedMs = aboveSince_.time_since_epoch().count() == 0 ? 0
        : (int)std::chrono::duration_cast<Millis>(now - aboveSince_).count();

    if (!s.features.autoClip) return false;   // meters still update; no clipping

    // Decide what fired: a user rule wins (explicit intent), else AI threshold.
    bool fire = false; bool wantReplay = false;
    ScoreReason reason;
    auto hit = rules_.evaluate(snap, now);
    if (hit) {
        fire = true;
        wantReplay = (hit->action == RuleAction::CreateReplay);
        reason.firedRuleName = hit->name;
        reason.primarySource = TriggerSource::Rule;
        reason.explanation = "Rule \"" + hit->name + "\" matched";
    } else if (s.features.aiScoring && confidence >= s.threshold) {
        fire = true;
        reason.primarySource = layers.voice >= layers.audio ? TriggerSource::MicVoice : TriggerSource::GameAudio;
        reason.explanation = "AI confidence " + std::to_string((int)confidence) +
                             "% ≥ threshold " + std::to_string((int)s.threshold) + "%";
    }
    if (!fire) return false;

    int modalities = 0;
    if (snap.micExcitement > 10) ++modalities;
    if (snap.gameIntensity > 10) ++modalities;
    if (snap.visionScore  > 10 || snap.killFeed) ++modalities;

    if (s.falsePositiveLearning && learner_.shouldReject(snap, sustainedMs, modalities)) {
        HC_INFO("Rejected likely false-positive (mods=%d sustained=%dms)", modalities, sustainedMs);
        return false;
    }

    out = HighlightEvent{};
    out.confidence  = std::max(confidence, hit ? hit->strength : 0.0f);
    out.rawScore    = layers.audio + layers.voice + layers.vision + layers.history;
    out.type        = momentum_.escalateType(bestType);
    out.triggeredAt = now;
    out.contributors= std::move(contributors);
    out.majorEvent  = out.confidence >= 85.0f || out.type == HighlightType::Ace ||
                      out.type == HighlightType::TeamWipe || out.type == HighlightType::Victory ||
                      out.type == HighlightType::MatchWinningPlay;
    out.wantReplay  = (wantReplay || out.majorEvent) && s.features.instantReplay;

    reason.audioScore  = layers.audio;
    reason.voiceScore  = layers.voice;
    reason.visionScore = layers.vision;
    reason.momentum    = snap.momentum;
    reason.finalScore  = out.confidence;
    out.reason = reason;

    if (!passesFailsafes(out, now)) return false;
    lastClipTime_ = now; lastClipType_ = out.type;
    HC_INFO("FIRED %s @ %.0f%% [%s]", to_string(out.type), out.confidence,
            reason.firedRuleName.empty() ? "AI" : reason.firedRuleName.c_str());
    return true;
}

} // namespace hypeclip
