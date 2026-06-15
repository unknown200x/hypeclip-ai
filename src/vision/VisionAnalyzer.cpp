#include "vision/VisionAnalyzer.hpp"
#include "vision/GameProfile.hpp"
#include "core/EventBus.hpp"
#include "core/Log.hpp"

#include <chrono>
#include <thread>
#include <mutex>

namespace hypeclip {

VisionAnalyzer::VisionAnalyzer() : profile_(std::make_shared<GameProfile>(GameProfile::generic())) {}
VisionAnalyzer::~VisionAnalyzer() { stop(); }

void VisionAnalyzer::setProfile(std::shared_ptr<GameProfile> p) {
    if (p) { profile_ = std::move(p); detectedGame_ = profile_->name; }
}
std::string VisionAnalyzer::detectedGame() const { return detectedGame_; }

void VisionAnalyzer::start(const VisionConfig& cfg) {
    if (running_.exchange(true)) return;
    cfg_ = cfg;
    HC_INFO("VisionAnalyzer start (fps=%d, onnx=%d, ep=%s)",
            cfg.analysisFps, (int)cfg.useOnnx, cfg.onnxExecutionProvider.c_str());
#if defined(HYPECLIP_VISION) && defined(HYPECLIP_HAVE_OBS)
    // obs_add_main_render_callback(&VisionAnalyzer::renderTap, this);
    // (registered by PipelineController, which owns the OBS lifetime)
#endif
    worker_ = std::thread(&VisionAnalyzer::workerLoop, this);
}

void VisionAnalyzer::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

// The worker dequeues GPU-readback frames, runs region heuristics / ONNX, and
// publishes Vision contributions. Frame acquisition (renderTap + stagesurf
// readback) and the ONNX session are the platform-specific pieces implemented
// for Windows/DirectML; here we model the cadence and contribution shape.
void VisionAnalyzer::workerLoop() {
    using namespace std::chrono;
    const auto period = milliseconds(std::max(1, 1000 / std::max(1, cfg_.analysisFps)));
    auto& bus = EventBus::instance();

    while (running_.load(std::memory_order_relaxed)) {
        // --- per-frame detection happens here ---
        // 1) pop latest downscaled frame from the readback queue (skip if none)
        // 2) for each rule in profile_->rules, sample its region:
        //      - killfeed: measure inter-frame churn in the region (new rows)
        //      - victory/ace/mvp: ONNX classify or template/color match
        // 3) on a positive, publish a Vision Contribution:
        //
        //    Contribution c;
        //    c.source = TriggerSource::Vision;
        //    c.type   = rule.type;
        //    c.score  = rule.baseScore;          // weighted later
        //    c.energy = confidence01;
        //    c.label  = profile_->name + ":" + rule.name;
        //    bus.publish(c);
        //
        // The framework above is wired; the GPU readback + ONNX inference are
        // the Windows-only modules documented in SPEC.md §Vision. Until those
        // assets are present this loop idles, so enabling vision never destabil-
        // ises the audio path.
        (void)bus;
        std::this_thread::sleep_for(period);
    }
}

#if defined(HYPECLIP_VISION) && defined(HYPECLIP_HAVE_OBS)
void VisionAnalyzer::renderTap(void* /*data*/, uint32_t /*cx*/, uint32_t /*cy*/) {
    // gs_stage_texture(...) the program target into a staging surface, then
    // gs_stagesurface_map() on the worker. Must be quick + must not stall GPU.
}
#endif

} // namespace hypeclip
