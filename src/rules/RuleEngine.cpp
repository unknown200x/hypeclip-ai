#include "rules/RuleEngine.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

void RuleEngine::setRules(std::vector<Rule> rules) { rules_ = std::move(rules); }

float RuleEngine::metricValue(Metric metric, const MetricSnapshot& m) {
    switch (metric) {
        case Metric::MicExcitement: return m.micExcitement;
        case Metric::MicVolume:     return m.micLevel;
        case Metric::MicPeak:       return m.micPeak;
        case Metric::ScreamScore:   return m.screamScore;
        case Metric::LaughterScore: return m.laughterScore;
        case Metric::ReactionScore: return m.reactionScore;
        case Metric::GameSpike:     return m.gameSpike;
        case Metric::GameIntensity: return m.gameIntensity;
        case Metric::GameLevel:     return m.gameLevel;
        case Metric::VisionScore:   return m.visionScore;
        case Metric::KillFeed:      return m.killFeed ? 100.0f : 0.0f;
        case Metric::Momentum:      return m.momentum;
        case Metric::Confidence:    return m.confidence;
    }
    return 0.0f;
}

bool RuleEngine::conditionMet(const RuleCondition& c, const MetricSnapshot& m) {
    const float v = metricValue(c.metric, m);
    switch (c.op) {
        case Comparator::Greater:      return v >  c.value;
        case Comparator::GreaterEqual: return v >= c.value;
        case Comparator::Less:         return v <  c.value;
        case Comparator::LessEqual:    return v <= c.value;
    }
    return false;
}

// How far past (or short of) the threshold, normalised 0..100.
float RuleEngine::conditionProgress(const RuleCondition& c, const MetricSnapshot& m) {
    const float v = metricValue(c.metric, m);
    const bool greater = c.op == Comparator::Greater || c.op == Comparator::GreaterEqual;
    if (c.metric == Metric::KillFeed) return conditionMet(c, m) ? 100.f : 0.f;
    float p;
    if (greater) p = c.value <= 0 ? 100.f : (v / c.value) * 100.f;
    else         p = v <= 0 ? 100.f : (c.value / std::max(v, 0.01f)) * 100.f;
    return std::clamp(p, 0.0f, 100.0f);
}

std::optional<RuleHit> RuleEngine::evaluate(const MetricSnapshot& m, TimePoint now) {
    for (size_t i = 0; i < rules_.size(); ++i) {
        const Rule& r = rules_[i];
        if (!r.enabled || r.conditions.empty()) continue;

        bool satisfied;
        if (r.join == RuleJoin::All) {
            satisfied = std::all_of(r.conditions.begin(), r.conditions.end(),
                        [&](const RuleCondition& c){ return conditionMet(c, m); });
        } else {
            satisfied = std::any_of(r.conditions.begin(), r.conditions.end(),
                        [&](const RuleCondition& c){ return conditionMet(c, m); });
        }
        if (!satisfied) continue;

        auto it = lastFired_.find((int)i);
        if (it != lastFired_.end() && now - it->second < Millis(r.cooldownMs)) continue;
        lastFired_[(int)i] = now;

        float strength = 0;
        for (const auto& c : r.conditions) strength = std::max(strength, conditionProgress(c, m));
        return RuleHit{(int)i, r.name, r.action, std::clamp(strength, 0.0f, 100.0f)};
    }
    return std::nullopt;
}

float RuleEngine::bestProgress(const MetricSnapshot& m) const {
    float best = 0;
    for (const auto& r : rules_) {
        if (!r.enabled || r.conditions.empty()) continue;
        // For ALL rules, progress is the minimum condition progress (weakest link);
        // for ANY rules, the maximum.
        float agg = (r.join == RuleJoin::All) ? 100.f : 0.f;
        for (const auto& c : r.conditions) {
            const float p = conditionProgress(c, m);
            agg = (r.join == RuleJoin::All) ? std::min(agg, p) : std::max(agg, p);
        }
        best = std::max(best, agg);
    }
    return best;
}

} // namespace hypeclip
