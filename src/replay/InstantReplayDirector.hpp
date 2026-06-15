#pragma once
// Flagship feature: on a major event, cut to a broadcast-style INSTANT REPLAY
// of the clip that was just saved, then return to live.
//
// Implementation strategy (no external video pipeline required):
//   * Maintain a dedicated, plugin-owned scene: "HypeClip Instant Replay".
//   * It contains a `ffmpeg_source` (media) that we point at the saved clip,
//     a stinger/transition, a text source for the lower-third, and per-style
//     overlay sources.
//   * Slow-motion is achieved via the media source `speed_percent` property;
//     freeze-frame via pausing playback on a marker; zoom/punch-in via an
//     animated transform on the media scene-item.
//   * On clip end (media-ended signal) we transition back to the live scene.
//
// Cinematic extras (motion blur, particles, dynamic camera) are implemented as
// pluggable OverlayEffect modules; v1 ships Esports/Hype/Streamer overlays and
// declares Cinematic/Retro as effect slots (see applyStyle()).
#include "core/Types.hpp"
#include "core/Config.hpp"
#include <atomic>
#include <string>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs.h>            // defines calldata_t used by the media-ended callback
#endif

namespace hypeclip {

class InstantReplayDirector {
public:
    void init();        // builds the replay scene + sources if missing
    void shutdown();

    // Cut to a styled slow-motion replay of `clip`, then auto-return to live.
    // Non-blocking; serialised so two replays never overlap (failsafe).
    void playReplay(const ClipRecord& clip, ReplayStyle style);

    bool isPlaying() const { return playing_.load(); }

private:
    void buildSceneIfNeeded();
    void applyStyle(ReplayStyle style, const ClipRecord& clip);
    void returnToLive();

#if defined(HYPECLIP_FRONTEND)
    static void mediaEnded(void* data, calldata_t* cd);
#endif

    std::atomic<bool> playing_{false};
    std::string liveSceneName_;     // remembered so we can switch back
    void* replayScene_ = nullptr;   // obs_source_t* (scene) — owned
    void* mediaSource_ = nullptr;   // obs_source_t* (ffmpeg_source)
    void* lowerThird_  = nullptr;   // obs_source_t* (text)
};

// Per-style presets (transition, speed ramp, overlay set, caption font).
struct ReplayStylePreset {
    const char* displayName;
    int   slowMoPercent;     // media speed_percent (e.g. 40 == 0.4x)
    bool  freezeOnImpact;
    bool  zoomPunchIn;
    const char* transition;  // OBS transition id
    const char* captionText; // default banner
};
ReplayStylePreset presetFor(ReplayStyle s);

} // namespace hypeclip
