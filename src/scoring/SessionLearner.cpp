#include "scoring/SessionLearner.hpp"
#include <algorithm>

namespace hypeclip {

void SessionLearner::observe(const MetricSnapshot& m, TimePoint now) {
    last_ = now;
    // Slowly track the "resting" level of each modality. Only adapt downward-ish
    // levels (ignore obvious hype) so the floor represents normal background.
    if (m.micExcitement < micNoiseFloor_ + 25.0f)
        micNoiseFloor_ += 0.01f * (m.micExcitement - micNoiseFloor_);
    if (m.gameIntensity < gameNoiseFloor_ + 25.0f)
        gameNoiseFloor_ += 0.01f * (m.gameIntensity - gameNoiseFloor_);
    micNoiseFloor_  = std::clamp(micNoiseFloor_, 5.0f, 60.0f);
    gameNoiseFloor_ = std::clamp(gameNoiseFloor_, 5.0f, 60.0f);
}

bool SessionLearner::shouldReject(const MetricSnapshot& m, int sustainedMs,
                                  int activeModalities) const {
    // Strong, corroborated, or clearly on-screen moments are never rejected.
    if (activeModalities >= 2) return false;
    if (m.killFeed) return false;
    if (m.confidence >= 90) return false;

    // A real solo-mic reaction is sustained and well above the learned noise floor.
    const bool sustained = sustainedMs >= 250;
    const bool wayAboveFloor = m.micExcitement > micNoiseFloor_ + 45.0f;

    // Cough / pop / desk bump: a brief high PEAK but little sustained excitement
    // and no reaction build-up.
    const bool transientPop = m.micPeak > 70 && !sustained && m.reactionScore < 30;

    // Notification / alert / music peak on game audio with no mic reaction.
    const bool loneGameBlip = m.gameSpike > 50 && m.micExcitement < 25 &&
                              m.gameIntensity < gameNoiseFloor_ + 15.0f;

    if (transientPop || loneGameBlip) return true;

    // Otherwise reject only weak, unsustained, near-floor single-modality blips.
    if (!sustained && !wayAboveFloor && m.reactionScore < 35) return true;
    return false;
}

void SessionLearner::reset() { micNoiseFloor_ = 20.0f; gameNoiseFloor_ = 15.0f; }

} // namespace hypeclip
