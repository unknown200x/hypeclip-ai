#include "audio/GameAudioDetector.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

namespace {
    constexpr float kBaseAlpha   = 0.002f;
    constexpr float kSmoothAlpha = 0.2f;
    constexpr auto  kMinGap      = Millis(500);
}

std::optional<Contribution> GameAudioDetector::feed(const FrameFeatures& f, TimePoint now) {
    const float energy = f.lowEnergy + f.highEnergy + f.voiceEnergy;
    baseEnergy_ += kBaseAlpha * (energy - baseEnergy_);
    const float base = std::max(baseEnergy_, 0.01f);
    const float ratio = energy / base;

    // Transient = high crest factor + strong spectral flux (a sharp onset like
    // a gunshot or explosion). A *run* of transients => a real firefight.
    const bool transient = f.crest > 4.0f && f.flux > base * 1.5f;
    transientRun_ = transient ? std::min(transientRun_ + 1, 40) : std::max(transientRun_ - 1, 0);

    // Musical stinger: sustained tonal low/voice energy without spiky crest
    // (victory fanfare, round-win chord).
    const bool musical = ratio > 2.2f && f.crest < 2.5f && f.lowEnergy > base;
    musicalFrames_ = musical ? std::min(musicalFrames_ + 1, 60) : std::max(musicalFrames_ - 2, 0);

    smoothed_ += kSmoothAlpha * (std::clamp((ratio - 1.0f) / 4.0f, 0.0f, 1.0f) - smoothed_);

    if (now - lastEmit_ < kMinGap) return std::nullopt;

    float score = 0.0f;
    HighlightType type = HighlightType::Generic;
    std::string label;

    if (musicalFrames_ >= 25)        { score = 50.0f; type = HighlightType::Victory;   label = "victory/round stinger"; }
    else if (transientRun_ >= 6)     { score = 25.0f; type = HighlightType::Kill;      label = "sustained combat"; }
    else if (transientRun_ >= 3)     { score = 12.0f; label = "combat burst"; }
    else return std::nullopt;        // a single shot or single cough => ignore

    lastEmit_ = now;
    Contribution c;
    c.source = TriggerSource::GameAudio;
    c.type   = type;
    c.score  = score;
    c.energy = smoothed_;
    c.when   = now;
    c.label  = std::move(label);
    return c;
}

} // namespace hypeclip
