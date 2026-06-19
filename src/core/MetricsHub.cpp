#include "core/MetricsHub.hpp"

namespace hypeclip {

MetricsHub& MetricsHub::instance() { static MetricsHub h; return h; }

void MetricsHub::updateMic(float level, float excitement, float peak,
                           float scream, float laughter, float reaction) {
    std::lock_guard<std::mutex> lk(mtx_);
    snap_.micLevel = level; snap_.micExcitement = excitement; snap_.micPeak = peak;
    snap_.screamScore = scream; snap_.laughterScore = laughter; snap_.reactionScore = reaction;
}
void MetricsHub::updateGame(float level, float spike, float intensity) {
    std::lock_guard<std::mutex> lk(mtx_);
    snap_.gameLevel = level; snap_.gameSpike = spike; snap_.gameIntensity = intensity;
}
void MetricsHub::updateVision(float score, bool killFeed) {
    std::lock_guard<std::mutex> lk(mtx_);
    snap_.visionScore = score; snap_.killFeed = killFeed;
}
void MetricsHub::updateScoring(float momentum, float confidence, float clipWorthiness) {
    std::lock_guard<std::mutex> lk(mtx_);
    snap_.momentum = momentum; snap_.confidence = confidence; snap_.clipWorthiness = clipWorthiness;
}
MetricSnapshot MetricsHub::get() const { std::lock_guard<std::mutex> lk(mtx_); return snap_; }
void MetricsHub::reset() { std::lock_guard<std::mutex> lk(mtx_); snap_ = MetricSnapshot{}; }

} // namespace hypeclip
