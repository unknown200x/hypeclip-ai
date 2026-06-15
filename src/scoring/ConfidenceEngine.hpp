#pragma once
// Multi-layer confidence fusion.
//
//   Layer 1: Audio (game audio salience)
//   Layer 2: Voice excitement (mic)
//   Layer 3: Visual detection (optional, GPU)
//   Layer 4: Event history (density/recency of events)
//   Layer 5: Momentum (escalation)
//
// Single-layer hits are deliberately damped (one gunshot, one cough => low).
// Cross-modal agreement is rewarded (gunfight + yelling => high), implementing
// the "Combined Audio Intelligence" requirement. Output is 0..100 confidence.
#include "core/Types.hpp"
#include "core/Config.hpp"

namespace hypeclip {

struct LayerScores {
    float audio   = 0.0f;   // 0..100 contribution from game audio in window
    float voice   = 0.0f;
    float vision  = 0.0f;
    float history = 0.0f;
    float momentum= 0.0f;   // 0..1 from MomentumTracker
};

class ConfidenceEngine {
public:
    // Fuse weighted layer scores into a single 0..100 confidence.
    float fuse(const LayerScores& l, const Weights& w) const;

    // Number of independent modalities currently firing (audio/voice/vision).
    static int activeModalities(const LayerScores& l);
};

} // namespace hypeclip
