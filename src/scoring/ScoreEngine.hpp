#pragma once
// The brain. Subscribes to detector Contributions, maintains a sliding window,
// computes per-layer scores, fuses them via ConfidenceEngine + MomentumTracker,
// applies failsafes, and publishes a HighlightEvent when threshold is crossed.
//
// Runs on whichever thread publishes contributions; internal state is mutex
// guarded. Evaluation is cheap (sum over a few deque entries).
#include "core/Types.hpp"
#include "scoring/MomentumTracker.hpp"
#include "scoring/ConfidenceEngine.hpp"
#include <deque>
#include <mutex>

namespace hypeclip {

class ScoreEngine {
public:
    ScoreEngine();

    // Wire up: subscribe to EventBus contributions.
    void start();
    void stop();

    // Exposed for the UI hype meter / clip-worthiness readout (0..100).
    float currentConfidence() const;

    // Visible for unit tests (no EventBus needed).
    void  ingest(const Contribution& c);
    float evaluate(TimePoint now, HighlightEvent& out, bool& fired);

private:
    void pruneOldEntries(TimePoint now);
    bool passesFailsafes(const HighlightEvent& e, TimePoint now);

    mutable std::mutex mtx_;
    std::deque<Contribution> window_;
    MomentumTracker momentum_;
    ConfidenceEngine confidence_;

    TimePoint lastClipTime_{};
    HighlightType lastClipType_ = HighlightType::Generic;
    float lastConfidence_ = 0.0f;
    uint64_t busToken_ = 0;

    static constexpr auto kWindow = Millis(4000);  // scoring window
};

} // namespace hypeclip
