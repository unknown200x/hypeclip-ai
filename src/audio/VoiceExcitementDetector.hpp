#pragma once
// Detects vocal excitement from the MIC stream: shouting, screaming, laughter,
// sudden vocal spikes, sustained celebration.
//
// Design principle: NO training and NO speech-to-text. We use an adaptive
// running baseline of the speaker's normal level so "LET'S GOOOO" reliably
// stands out from calm talking regardless of mic gain or room — i.e. it works
// out of the box for anyone.
#include "audio/AudioFeatures.hpp"
#include "core/Types.hpp"
#include <optional>

namespace hypeclip {

class VoiceExcitementDetector {
public:
    // Feed one analysis frame (≈20 ms). Returns a Contribution when the frame
    // (or sustained run) is judged exciting; std::nullopt otherwise.
    std::optional<Contribution> feed(const FrameFeatures& f, TimePoint now);

    // Current normalized excitement 0..1 (drives the live Hype Meter even when
    // below clip threshold).
    float excitement() const { return smoothedExcite_; }

private:
    // Exponential moving baselines (slow) and short-term (fast) trackers.
    float baseRms_   = 0.02f;   // adapts to ambient/normal speaking level
    float baseVoice_ = 0.0f;
    float fastRms_   = 0.0f;
    float smoothedExcite_ = 0.0f;

    int   sustainedFrames_ = 0; // consecutive exciting frames (celebration runs)
    int   laughterFrames_  = 0; // rhythmic voiced bursts -> laughter
    float prevZcr_ = 0.0f;
    TimePoint lastEmit_{};
};

} // namespace hypeclip
