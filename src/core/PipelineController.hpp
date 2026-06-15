#pragma once
// Owns and wires every subsystem. One instance for the whole plugin.
//
//   AudioCapture(mic) ─┐
//   AudioCapture(game)─┼─▶ EventBus(Contribution) ─▶ ScoreEngine ─▶ EventBus(Highlight)
//   VisionAnalyzer ────┘                                              │
//                                                                     ▼
//        ClipManager ◀── ReplayBufferController       InstantReplayDirector
//             │
//             ▼
//        EventBus(ClipRecord) ─▶ StorageManager + UI dock
//
#include "audio/AudioCapture.hpp"
#include "scoring/ScoreEngine.hpp"
#include "replay/ReplayBufferController.hpp"
#include "replay/InstantReplayDirector.hpp"
#include "replay/ClipManager.hpp"
#include "storage/StorageManager.hpp"
#include "vision/VisionAnalyzer.hpp"
#include "core/Config.hpp"

#include <memory>

namespace hypeclip {

class PipelineController {
public:
    static PipelineController& instance();

    void startup();    // called from obs_module_load (post-frontend-ready)
    void shutdown();

    // Re-read Config and (re)attach sources / toggle vision. Hot-applies.
    void reconfigure();

    // UI accessors (thread-safe enough for a meter poll).
    float micLevel()  const { return mic_  ? mic_->meterLevel()  : 0.f; }
    float gameLevel() const { return game_ ? game_->meterLevel() : 0.f; }
    float confidence() const { return score_.currentConfidence(); }
    std::vector<ClipRecord> clips() const { return clipMgr_ ? clipMgr_->recentClips() : std::vector<ClipRecord>{}; }

    ReplayBufferController& replay() { return rb_; }
    StorageManager&         storage() { return storage_; }

private:
    PipelineController() = default;

    std::unique_ptr<AudioCapture> mic_;
    std::unique_ptr<AudioCapture> game_;
    ScoreEngine              score_;
    ReplayBufferController    rb_;
    InstantReplayDirector     director_;
    std::unique_ptr<ClipManager> clipMgr_;
    StorageManager            storage_;
    VisionAnalyzer            vision_;
    bool started_ = false;
};

} // namespace hypeclip
