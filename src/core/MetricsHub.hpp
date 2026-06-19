#pragma once
// Single thread-safe live MetricSnapshot shared by detectors (writers), the rule
// engine + AI scorer (readers), and the UI meters (readers). Cheap mutex; the UI
// polls at ~10 Hz and detectors update at ~50 Hz, so contention is negligible.
#include "core/Types.hpp"
#include <mutex>

namespace hypeclip {

class MetricsHub {
public:
    static MetricsHub& instance();

    // Partial updaters (each detector owns its fields).
    void updateMic(float level, float excitement, float peak,
                   float scream, float laughter, float reaction);
    void updateGame(float level, float spike, float intensity);
    void updateVision(float score, bool killFeed);
    void updateScoring(float momentum, float confidence, float clipWorthiness);

    MetricSnapshot get() const;
    void reset();

private:
    MetricsHub() = default;
    mutable std::mutex mtx_;
    MetricSnapshot snap_;
};

} // namespace hypeclip
