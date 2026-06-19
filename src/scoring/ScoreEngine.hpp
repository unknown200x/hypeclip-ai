#pragma once
// The decision brain (v2). Fuses the AI scorer + the user rule engine, writes the
// live MetricSnapshot, applies the false-positive learner and failsafes, and emits
// a fully-explained HighlightEvent. Honors master + per-feature toggles.
#include "core/Types.hpp"
#include "scoring/MomentumTracker.hpp"
#include "scoring/ConfidenceEngine.hpp"
#include "scoring/SessionLearner.hpp"
#include "rules/RuleEngine.hpp"
#include <deque>
#include <mutex>

namespace hypeclip {

class ScoreEngine {
public:
    ScoreEngine();
    void start();
    void stop();
    void reloadConfig();   // re-pull rules + flags after settings change

    float currentConfidence() const;

    // Visible for unit tests.
    void  ingest(const Contribution& c);
    bool  evaluate(TimePoint now, HighlightEvent& out);

private:
    void pruneOldEntries(TimePoint now);
    bool passesFailsafes(const HighlightEvent& e, TimePoint now);

    mutable std::mutex mtx_;
    std::deque<Contribution> window_;
    MomentumTracker  momentum_;
    ConfidenceEngine confidence_;
    SessionLearner   learner_;
    RuleEngine       rules_;

    TimePoint lastClipTime_{};
    HighlightType lastClipType_ = HighlightType::Generic;
    float   lastConfidence_ = 0.0f;
    TimePoint aboveSince_{};
    uint64_t busToken_ = 0;

    static constexpr auto kWindow = Millis(4000);
};

} // namespace hypeclip
