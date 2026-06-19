#pragma once
// Priority #2/#3 — broadcast-clean instant replay.
//
//   * NO captions, commentary, hype text or kill callouts. The ONLY on-screen
//     text is a single centered "INSTANT REPLAY" (toggleable).
//   * The user chooses the replay scene (any OBS scene) OR we use a minimal
//     plugin-owned scene (media + the centered label).
//   * Enter/return transitions and durations are user-configurable and use the
//     native OBS transitions the user already has.
#include "core/Types.hpp"
#include "core/Config.hpp"
#include <atomic>
#include <string>
#include <thread>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs.h>
#endif

namespace hypeclip {

class InstantReplayDirector {
public:
    void init();
    void shutdown();

    // Cut to the configured replay scene with the clip, hold for the configured
    // duration, then return to live. Non-blocking; never overlaps (failsafe).
    void playReplay(const ClipRecord& clip);

    bool isPlaying() const { return playing_.load(); }

private:
    void buildMinimalSceneIfNeeded();   // plugin-owned clean scene
    void returnToLive();

#if defined(HYPECLIP_HAVE_OBS)
    obs_source_t* resolveReplayScene(const ReplayConfig& rc);
    void applyTransition(const std::string& name, int durationMs);
#endif

    std::atomic<bool> playing_{false};
    std::string liveSceneName_;
    void* minimalScene_ = nullptr;   // obs_source_t* (scene) — owned
    void* mediaSource_  = nullptr;   // obs_source_t* (ffmpeg_source) — owned
    void* label_        = nullptr;   // obs_source_t* (text) — owned
    std::thread timer_;
};

} // namespace hypeclip
