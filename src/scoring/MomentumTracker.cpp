#include "scoring/MomentumTracker.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

void MomentumTracker::decayTo(TimePoint now) {
    if (last_.time_since_epoch().count() == 0) { last_ = now; return; }
    const float dt = std::chrono::duration<float>(now - last_).count();
    if (dt > 0) momentum_ *= std::exp(-kDecayPerSec * dt);
    last_ = now;

    while (!events_.empty() && now - events_.front() > kWindow)
        events_.pop_front();
}

void MomentumTracker::add(const Contribution& c) {
    decayTo(c.when);

    // Clustering amplifier: the more recent events stacked, the more each adds.
    const float cluster = 1.0f + 0.35f * (float)events_.size();
    momentum_ = std::clamp(momentum_ + c.energy * 0.25f * cluster, 0.0f, 1.0f);

    if (c.source == TriggerSource::GameAudio || c.source == TriggerSource::Vision)
        events_.push_back(c.when);
}

HighlightType MomentumTracker::escalateType(HighlightType base) const {
    const int n = recentEventCount();
    const bool combat = base == HighlightType::Kill || base == HighlightType::MultiKill ||
                        base == HighlightType::Generic;
    if (!combat) return base;          // don't override Victory/Clutch etc.
    if (n >= 5 && momentum_ > 0.85f) return HighlightType::Ace;
    if (n >= 3)                      return HighlightType::MultiKill;
    if (n >= 2)                      return HighlightType::KillStreak;
    return base;
}

} // namespace hypeclip
