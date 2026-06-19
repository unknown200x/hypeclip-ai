#pragma once
// Game-audio analysis (v2). Adaptive baseline; produces 0..100 sub-scores
// (level / spike / intensity) for the meters, rule engine and tuning controls.
#include "audio/AudioFeatures.hpp"
#include "core/Types.hpp"

namespace hypeclip {

struct GameScores {
    float level     = 0;  // loudness 0..100
    float spike     = 0;  // transient/onset strength 0..100
    float intensity = 0;  // smoothed combat intensity 0..100
    int   transientRun = 0;
    bool  musicalStinger = false;  // victory/round jingle detected
};

class GameAudioDetector {
public:
    // `spikeSensitivity`/`clustering` 0..100 from GameAudioTuning let the user
    // bias transient detection without recompiling.
    GameScores analyze(const FrameFeatures& f, float spikeSensitivity, float clustering);

private:
    float baseEnergy_ = 0.01f, smoothed_ = 0.0f;
    int   transientRun_ = 0, musicalFrames_ = 0;
};

} // namespace hypeclip
