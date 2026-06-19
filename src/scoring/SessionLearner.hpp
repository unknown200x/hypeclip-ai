#pragma once
// Priority #9 — false-positive prevention that adapts to the session.
//
// Genuine hype is sustained and/or corroborated across modalities. Coughs, mic
// pops, desk bumps, keyboard slams, Discord pings and alert sounds are short,
// isolated, single-modality transients. We learn the session's "noise" level and
// reject candidates that match it. Pure logic, unit-tested, no OBS.
#include "core/Types.hpp"

namespace hypeclip {

class SessionLearner {
public:
    // Feed every analysis tick so the learner can adapt baselines (cheap).
    void observe(const MetricSnapshot& m, TimePoint now);

    // Decide whether a candidate clip is likely a false positive.
    //  sustainedMs      : how long the trigger condition has held
    //  activeModalities : how many of mic/game/vision currently fire
    // Returns true => REJECT (do not clip).
    bool shouldReject(const MetricSnapshot& m, int sustainedMs, int activeModalities) const;

    float noiseFloor() const { return micNoiseFloor_; }
    void reset();

private:
    float micNoiseFloor_  = 20.0f;   // adaptive typical "non-hype" mic excitement
    float gameNoiseFloor_ = 15.0f;
    TimePoint last_{};
};

} // namespace hypeclip
