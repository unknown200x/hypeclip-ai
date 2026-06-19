#pragma once
// Owns + wires every subsystem and enforces the master + per-feature toggles
// (Priority #1). When the master switch is OFF, all analysis threads are stopped
// and no CPU/GPU is used beyond idle.
#include "audio/AudioCapture.hpp"
#include "scoring/ScoreEngine.hpp"
#include "replay/ReplayBufferController.hpp"
#include "replay/InstantReplayDirector.hpp"
#include "replay/ClipManager.hpp"
#include "storage/StorageManager.hpp"
#include "vision/VisionAnalyzer.hpp"
#include "core/Config.hpp"

#include <memory>
#include <vector>

namespace hypeclip {

class PipelineController {
public:
    static PipelineController& instance();

    void startup();
    void shutdown();
    void reconfigure();          // hot-apply settings (toggles, sources, rules)
    bool isActive() const { return audioRunning_; }

    float confidence() const { return score_.currentConfidence(); }
    std::vector<ClipRecord> clips() const { return clipMgr_ ? clipMgr_->recentClips() : std::vector<ClipRecord>{}; }

    ReplayBufferController& replay() { return rb_; }
    StorageManager&         storage() { return storage_; }

private:
    PipelineController() = default;
    void startAudio(const Settings& s);
    void stopAudio();

    std::vector<std::unique_ptr<AudioCapture>> mics_;
    std::vector<std::unique_ptr<AudioCapture>> games_;
    ScoreEngine             score_;
    ReplayBufferController   rb_;
    InstantReplayDirector    director_;
    std::unique_ptr<ClipManager> clipMgr_;
    StorageManager           storage_;
    VisionAnalyzer           vision_;
    bool started_ = false;
    bool audioRunning_ = false;
};

} // namespace hypeclip
