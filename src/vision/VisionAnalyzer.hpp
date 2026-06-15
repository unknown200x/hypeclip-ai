#pragma once
// OPTIONAL GPU computer-vision layer (off by default).
//
// Watches the program video at a low cadence (e.g. 4–6 fps, NOT every frame),
// downscales on the GPU, and runs cheap template/heuristic detectors plus an
// optional ONNX classifier to spot on-screen events: kill-feed changes, victory
// screens, multi-kill banners, knockdowns, MVP/ACE indicators, damage spikes.
//
// It NEVER blocks the render thread: frames are pulled via a scenecapture/
// stagesurf readback into a queue and processed on a worker that may use
// DirectML / CUDA. Results are published as TriggerSource::Vision Contributions.
//
// v1 ships the framework + a game-agnostic heuristic detector (rapid lower-
// right text churn = kill feed; full-screen color/text burst = victory). Game
// profiles (Valorant, Apex, CS2, ...) plug in template regions + an ONNX model
// path; see GameProfile.
#include "core/Types.hpp"
#include <atomic>
#include <thread>
#include <memory>
#include <string>

namespace hypeclip {

class GameProfile;

struct VisionConfig {
    int   analysisFps   = 5;      // frames analysed per second
    int   downscaleW    = 480;    // GPU downscale target width
    bool  useOnnx       = false;  // load per-game ONNX model if available
    std::string onnxExecutionProvider = "DirectML"; // or "CUDA"/"CPU"
};

class VisionAnalyzer {
public:
    VisionAnalyzer();
    ~VisionAnalyzer();

    void start(const VisionConfig& cfg);
    void stop();
    bool isRunning() const { return running_.load(); }

    // Switch active game profile (also used to set the auto-detected game name).
    void setProfile(std::shared_ptr<GameProfile> profile);
    std::string detectedGame() const;

#if defined(HYPECLIP_VISION) && defined(HYPECLIP_HAVE_OBS)
    // Render-thread tap: copies the program frame to a GPU stage surface and
    // enqueues for the worker. Registered via obs_add_main_render_callback.
    static void renderTap(void* data, uint32_t cx, uint32_t cy);
#endif

private:
    void workerLoop();

    VisionConfig cfg_;
    std::shared_ptr<GameProfile> profile_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::string detectedGame_;
};

} // namespace hypeclip
