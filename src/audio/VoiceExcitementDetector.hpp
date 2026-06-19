#pragma once
// Mic analysis (v2). Training-free, adaptive-baseline. Produces granular 0..100
// sub-scores the user can threshold independently (Priority #5) and that feed the
// live meters + rule engine. Emission decisions live in AudioCapture so they can
// read the user's thresholds + feature toggles.
#include "audio/AudioFeatures.hpp"
#include "core/Types.hpp"

namespace hypeclip {

struct MicScores {
    float level      = 0;  // loudness 0..100
    float excitement = 0;  // above-baseline 0..100
    float peak       = 0;  // short-term peak 0..100
    float scream     = 0;  // vocal strain 0..100
    float laughter   = 0;  // rhythmic bursts 0..100
    float reaction   = 0;  // sustained reaction energy 0..100
    int   sustainedMs = 0; // how long excitement has held
};

class VoiceExcitementDetector {
public:
    MicScores analyze(const FrameFeatures& f, TimePoint now);

private:
    float baseRms_ = 0.02f, baseVoice_ = 0.0f, fastRms_ = 0.0f;
    float smoothedExcite_ = 0.0f;
    int   sustainedFrames_ = 0, laughterFrames_ = 0;
    float prevZcr_ = 0.0f;
};

} // namespace hypeclip
