#pragma once
// Detects salient GAME-audio events from the desktop/app/source stream:
// explosions, gunfire bursts, kill-confirm stings, victory/round-win jingles.
//
// Like the voice detector this is training-free and uses adaptive baselines so
// it adjusts to per-game mix levels automatically.
#include "audio/AudioFeatures.hpp"
#include "core/Types.hpp"
#include <optional>

namespace hypeclip {

class GameAudioDetector {
public:
    std::optional<Contribution> feed(const FrameFeatures& f, TimePoint now);
    float intensity() const { return smoothed_; }

private:
    float baseEnergy_ = 0.01f;
    float smoothed_   = 0.0f;
    int   transientRun_ = 0;   // consecutive sharp onsets => gunfight, not one shot
    int   musicalFrames_ = 0;  // sustained tonal energy => victory jingle/stinger
    TimePoint lastEmit_{};
};

} // namespace hypeclip
