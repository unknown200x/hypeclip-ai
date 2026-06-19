#include "audio/GameAudioDetector.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

namespace {
    constexpr float kBaseAlpha   = 0.002f;
    constexpr float kSmoothAlpha = 0.2f;
    inline float pct(float x) { return std::clamp(x, 0.0f, 1.0f) * 100.0f; }
}

GameScores GameAudioDetector::analyze(const FrameFeatures& f, float spikeSensitivity,
                                      float clustering) {
    const float energy = f.lowEnergy + f.highEnergy + f.voiceEnergy;
    baseEnergy_ += kBaseAlpha * (energy - baseEnergy_);
    const float base  = std::max(baseEnergy_, 0.01f);
    const float ratio = energy / base;

    // Higher sensitivity -> lower crest/flux bar for counting a transient.
    const float crestBar = 5.0f - (spikeSensitivity / 100.0f) * 3.0f;     // 5..2
    const float fluxBar  = base * (2.0f - (spikeSensitivity / 100.0f));    // 2x..1x
    const bool transient = f.crest > crestBar && f.flux > fluxBar;

    const int runCap = 20 + (int)(clustering / 100.0f * 30.0f);            // 20..50
    transientRun_ = transient ? std::min(transientRun_ + 1, runCap)
                              : std::max(transientRun_ - 1, 0);

    const bool musical = ratio > 2.2f && f.crest < 2.5f && f.lowEnergy > base;
    musicalFrames_ = musical ? std::min(musicalFrames_ + 1, 60) : std::max(musicalFrames_ - 2, 0);

    smoothed_ += kSmoothAlpha * (std::clamp((ratio - 1.0f) / 4.0f, 0.0f, 1.0f) - smoothed_);

    GameScores s;
    s.level         = pct(energy / (base * 5.0f));
    s.spike         = transient ? pct(std::min(f.crest / 6.0f, 1.0f)) : pct(f.flux / (fluxBar + 1e-4f) * 0.4f);
    s.intensity     = pct(smoothed_);
    s.transientRun  = transientRun_;
    s.musicalStinger= musicalFrames_ >= 25;
    return s;
}

} // namespace hypeclip
