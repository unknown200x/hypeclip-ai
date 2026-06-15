#include "scoring/ConfidenceEngine.hpp"
#include <algorithm>
#include <cmath>

namespace hypeclip {

int ConfidenceEngine::activeModalities(const LayerScores& l) {
    int n = 0;
    if (l.audio  > 10.0f) ++n;
    if (l.voice  > 10.0f) ++n;
    if (l.vision > 10.0f) ++n;
    return n;
}

float ConfidenceEngine::fuse(const LayerScores& l, const Weights& w) const {
    const float audio  = l.audio  * w.gameAudio;
    const float voice  = l.voice  * w.micVoice;
    const float vision = l.vision * w.vision;
    const float hist   = l.history* w.eventHistory;

    // Base aggregate, capped so any one weak layer can't alone reach threshold.
    float base = audio + voice + vision + hist;

    // Cross-modal agreement bonus: independent modalities corroborating each
    // other is the strongest signal a moment is real (kills + yelling).
    const int mod = activeModalities(l);
    float agreement = 1.0f;
    if (mod >= 2) agreement += 0.25f * (float)(mod - 1);   // +25% per extra modality

    // Single-modality damping: lone signal (one stinger, one cough) stays low.
    if (mod <= 1) base *= 0.6f;

    // Momentum multiplier: escalation pushes confidence toward the ceiling.
    const float momentum = std::clamp(l.momentum, 0.0f, 1.0f) * w.momentum;
    const float withMomentum = base * agreement * (1.0f + 0.6f * momentum);

    return std::clamp(withMomentum, 0.0f, 100.0f);
}

} // namespace hypeclip
