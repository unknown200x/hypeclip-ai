#include "audio/VoiceExcitementDetector.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

namespace {
    constexpr float kBaseAlphaSlow = 0.0015f;
    constexpr float kFastAlpha     = 0.25f;
    constexpr float kSmoothAlpha   = 0.15f;
    constexpr float kSpikeRatio    = 3.0f;
    constexpr int   kFrameMs       = 20;
    inline float pct(float x) { return std::clamp(x, 0.0f, 1.0f) * 100.0f; }
}

MicScores VoiceExcitementDetector::analyze(const FrameFeatures& f, TimePoint /*now*/) {
    baseRms_   += kBaseAlphaSlow * (f.rms - baseRms_);
    baseVoice_ += kBaseAlphaSlow * (f.voiceEnergy - baseVoice_);
    fastRms_   += kFastAlpha * (f.rms - fastRms_);

    const float base   = std::max(baseRms_, 0.01f);
    const float ratio  = fastRms_ / base;
    const float bright = f.highEnergy / std::max(f.voiceEnergy, 1e-4f);

    const bool loudSpike = ratio > kSpikeRatio && f.voiceEnergy > baseVoice_ * 2.0f;
    const bool scream    = loudSpike && bright > 0.6f && f.peak > 0.5f;

    const bool burst = f.voiceEnergy > baseVoice_ * 1.8f && std::fabs(f.zcr - prevZcr_) > 0.05f;
    laughterFrames_ = burst ? std::min(laughterFrames_ + 1, 30) : std::max(laughterFrames_ - 2, 0);
    prevZcr_ = f.zcr;

    float instant = std::clamp((ratio - 1.0f) / (kSpikeRatio + 1.0f), 0.0f, 1.0f);
    if (scream) instant = std::max(instant, 0.95f);
    smoothedExcite_ += kSmoothAlpha * (instant - smoothedExcite_);

    sustainedFrames_ = (loudSpike || laughterFrames_ >= 8)
                       ? sustainedFrames_ + 1 : std::max(sustainedFrames_ - 1, 0);

    MicScores s;
    s.level       = pct(f.rms * 4.0f);                 // perceptual-ish loudness
    s.excitement  = pct(smoothedExcite_);
    s.peak        = pct(f.peak);
    s.scream      = scream ? pct(0.8f + bright * 0.2f) : pct(bright * 0.5f * (ratio / kSpikeRatio));
    s.laughter    = pct(laughterFrames_ / 15.0f);
    s.reaction    = pct(sustainedFrames_ / 18.0f);
    s.sustainedMs = sustainedFrames_ * kFrameMs;
    return s;
}

} // namespace hypeclip
