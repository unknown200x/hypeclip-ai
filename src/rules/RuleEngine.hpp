#pragma once
// Priority #7 — Streamer.bot-style IF/THEN rule engine. Pure logic, no OBS, so
// it is fully unit-tested. Evaluates user rules against the live MetricSnapshot.
#include "core/Types.hpp"
#include <optional>
#include <vector>
#include <map>

namespace hypeclip {

struct RuleHit {
    int          index = -1;
    std::string  name;
    RuleAction   action = RuleAction::CreateClip;
    float        strength = 0;     // 0..100, how strongly conditions were exceeded
};

class RuleEngine {
public:
    // Replace the active rule set (called when settings change).
    void setRules(std::vector<Rule> rules);

    // Evaluate at time `now`. Returns the first satisfied rule whose cooldown has
    // elapsed; std::nullopt otherwise. Updates internal cooldown bookkeeping.
    std::optional<RuleHit> evaluate(const MetricSnapshot& m, TimePoint now);

    // 0..100 — how close the closest enabled rule is to firing (drives the
    // Clip-Worthiness meter even before a rule trips).
    float bestProgress(const MetricSnapshot& m) const;

    size_t ruleCount() const { return rules_.size(); }

    // Visible for tests.
    static bool conditionMet(const RuleCondition& c, const MetricSnapshot& m);
    static float metricValue(Metric metric, const MetricSnapshot& m);

private:
    static float conditionProgress(const RuleCondition& c, const MetricSnapshot& m);
    std::vector<Rule> rules_;
    std::map<int, TimePoint> lastFired_;   // per-rule cooldown
};

} // namespace hypeclip
