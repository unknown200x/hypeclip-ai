#include "audio/VoiceExcitementDetector.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

// Tunables (Creator mode can override the multiplier via Config weights).
namespace {
    constexpr float kBaseAlphaSlow = 0.0015f;  // baseline adaptation (very slow)
    constexpr float kFastAlpha     = 0.25f;     // short-term envelope
    constexpr float kSmoothAlpha   = 0.15f;     // hype-meter smoothing
    constexpr float kSpikeRatio    = 3.0f;      // fast/base ratio that counts as a spike
    constexpr int   kSustainEmit   = 12;        // ~250 ms of sustained excitement
    constexpr auto  kMinGap        = Millis(600);
}

std::optional<Contribution> VoiceExcitementDetector::feed(const FrameFeatures& f, TimePoint now) {
    // Adapt baselines only on non-exciting, voiced-or-quiet frames.
    baseRms_   += kBaseAlphaSlow * (f.rms - baseRms_);
    baseVoice_ += kBaseAlphaSlow * (f.voiceEnergy - baseVoice_);
    fastRms_   += kFastAlpha * (f.rms - fastRms_);

    const float base = std::max(baseRms_, 0.01f);
    const float ratio = fastRms_ / base;                  // how much louder than normal
    const float bright = f.highEnergy / std::max(f.voiceEnergy, 1e-4f);

    // Spike: significantly louder than the speaker's baseline, voiced band hot.
    const bool loudSpike = ratio > kSpikeRatio && f.voiceEnergy > baseVoice_ * 2.0f;

    // Scream/yell: loud spike + lots of high-frequency energy (vocal strain).
    const bool scream = loudSpike && bright > 0.6f && f.peak > 0.5f;

    // Laughter: rhythmic voiced bursts -> oscillating ZCR + moderate loudness.
    const bool burst = f.voiceEnergy > baseVoice_ * 1.8f &&
                       std::fabs(f.zcr - prevZcr_) > 0.05f;
    laughterFrames_ = burst ? std::min(laughterFrames_ + 1, 30) : std::max(laughterFrames_ - 2, 0);
    prevZcr_ = f.zcr;
    const bool laughter = laughterFrames_ >= 8;

    // Normalized excitement for the meter.
    float instant = std::clamp((ratio - 1.0f) / (kSpikeRatio + 1.0f), 0.0f, 1.0f);
    if (scream) instant = std::max(instant, 0.95f);
    smoothedExcite_ += kSmoothAlpha * (instant - smoothedExcite_);

    sustainedFrames_ = (loudSpike || laughter) ? sustainedFrames_ + 1
                                               : std::max(sustainedFrames_ - 1, 0);

    // Decide whether to emit a Contribution.
    if (now - lastEmit_ < kMinGap) return std::nullopt;

    float score = 0.0f;
    HighlightType type = HighlightType::Reaction;
    std::string label;

    if (scream)                      { score = 45.0f; label = "mic scream"; }
    else if (sustainedFrames_ >= kSustainEmit) { score = 32.0f; label = "sustained excitement"; }
    else if (laughter)               { score = 30.0f; type = HighlightType::FunnyMoment; label = "laughter burst"; }
    else if (loudSpike)              { score = 22.0f; label = "voice spike"; }
    else return std::nullopt;

    lastEmit_ = now;
    Contribution c;
    c.source = TriggerSource::MicVoice;
    c.type   = type;
    c.score  = score;
    c.energy = smoothedExcite_;
    c.when   = now;
    c.label  = std::move(label);
    return c;
}

} // namespace hypeclip
