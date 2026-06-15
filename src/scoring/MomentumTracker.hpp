#pragma once
// Tracks "is something building toward something bigger?" over a short window.
//
// Single kill => low. Kill, kill, kill in quick succession => escalating.
// Ace / match-winning => maxed. We model momentum as a leaky integrator of
// recent contribution energy with a recency-weighted multiplier for clustered
// events, so rapid bursts amplify rather than just add.
#include "core/Types.hpp"
#include <deque>

namespace hypeclip {

class MomentumTracker {
public:
    void add(const Contribution& c);

    // Decays momentum to "now"; call before reading.
    void decayTo(TimePoint now);

    // 0..1 momentum factor. ConfidenceEngine multiplies score by (1 + k*momentum).
    float momentum() const { return momentum_; }

    // Count of distinct game-event contributions in the recent window — used to
    // promote a Kill into MultiKill / Ace classification.
    int recentEventCount() const { return (int)events_.size(); }

    HighlightType escalateType(HighlightType base) const;

private:
    float momentum_ = 0.0f;
    TimePoint last_{};
    std::deque<TimePoint> events_;  // game/vision event timestamps in window
    static constexpr float kDecayPerSec = 0.55f;   // leak rate
    static constexpr auto  kWindow = Millis(6000);
};

} // namespace hypeclip
