#include "replay/InstantReplayDirector.hpp"
#include "core/Log.hpp"

#if defined(HYPECLIP_FRONTEND)
#include <obs.h>
#include <obs-frontend-api.h>
#endif

namespace hypeclip {

static const char* kReplaySceneName = "HypeClip Instant Replay";

ReplayStylePreset presetFor(ReplayStyle s) {
    switch (s) {
        case ReplayStyle::Esports:
            return {"Esports", 45, true,  true,  "obs_stinger_transition", "INSTANT REPLAY"};
        case ReplayStyle::Hype:
            return {"Hype",    35, true,  true,  "swipe_transition",        "REPLAY"};
        case ReplayStyle::Cinematic:
            return {"Cinematic",30, false, true,  "fade_transition",        "REPLAY"};
        case ReplayStyle::Streamer:
            return {"Streamer", 50, false, false, "slide_transition",       "REPLAY!"};
        case ReplayStyle::Retro:
            return {"Retro",    60, true,  false, "cut_transition",         "REPLAY"};
    }
    return {"Esports", 45, true, true, "fade_transition", "INSTANT REPLAY"};
}

void InstantReplayDirector::init() {
#if defined(HYPECLIP_FRONTEND)
    buildSceneIfNeeded();
    HC_INFO("InstantReplayDirector ready");
#endif
}

void InstantReplayDirector::shutdown() {
#if defined(HYPECLIP_FRONTEND)
    if (mediaSource_) { obs_source_release((obs_source_t*)mediaSource_); mediaSource_ = nullptr; }
    if (lowerThird_)  { obs_source_release((obs_source_t*)lowerThird_);  lowerThird_  = nullptr; }
    if (replayScene_) { obs_source_release((obs_source_t*)replayScene_); replayScene_ = nullptr; }
#endif
}

#if defined(HYPECLIP_FRONTEND)

void InstantReplayDirector::buildSceneIfNeeded() {
    obs_source_t* existing = obs_get_source_by_name(kReplaySceneName);
    if (existing) { replayScene_ = existing; }
    else {
        obs_scene_t* scene = obs_scene_create(kReplaySceneName);
        // obs_source_get_ref adds a reference we own (obs_source_addref was removed
        // in newer OBS); released in shutdown().
        replayScene_ = obs_source_get_ref(obs_scene_get_source(scene));
    }

    if (!mediaSource_) {
        obs_data_t* set = obs_data_create();
        obs_data_set_bool(set, "looping", false);
        obs_data_set_bool(set, "restart_on_activate", true);
        mediaSource_ = obs_source_create("ffmpeg_source", "HypeClip Replay Media", set, nullptr);
        obs_data_release(set);
        // media-ended signal -> return to live
        signal_handler_t* sh = obs_source_get_signal_handler((obs_source_t*)mediaSource_);
        signal_handler_connect(sh, "media_ended", &InstantReplayDirector::mediaEnded, this);
        obs_scene_add(obs_scene_from_source((obs_source_t*)replayScene_), (obs_source_t*)mediaSource_);
    }

    if (!lowerThird_) {
        obs_data_t* set = obs_data_create();
        obs_data_set_string(set, "text", "INSTANT REPLAY");
#if defined(_WIN32)
        const char* textId = "text_gdiplus_v2";
#else
        const char* textId = "text_ft2_source_v2";
#endif
        lowerThird_ = obs_source_create(textId, "HypeClip Replay Caption", set, nullptr);
        obs_data_release(set);
        obs_scene_add(obs_scene_from_source((obs_source_t*)replayScene_), (obs_source_t*)lowerThird_);
    }
}

void InstantReplayDirector::applyStyle(ReplayStyle style, const ClipRecord& clip) {
    const ReplayStylePreset p = presetFor(style);

    // Point media source at the saved clip + apply slow-motion ramp.
    obs_data_t* set = obs_source_get_settings((obs_source_t*)mediaSource_);
    obs_data_set_string(set, "local_file", clip.filePath.c_str());
    obs_data_set_int(set, "speed_percent", p.slowMoPercent);
    obs_source_update((obs_source_t*)mediaSource_, set);
    obs_data_release(set);

    // Caption: combine style banner with the auto title.
    obs_data_t* cap = obs_source_get_settings((obs_source_t*)lowerThird_);
    std::string caption = std::string(p.captionText) + "  -  " + clip.title;
    obs_data_set_string(cap, "text", caption.c_str());
    obs_source_update((obs_source_t*)lowerThird_, cap);
    obs_data_release(cap);

    // NOTE: zoom punch-in (animated sceneitem transform), freeze-frame (pause at
    // marker), motion blur and particle overlays are implemented as OverlayEffect
    // modules driven on a render tick; see docs/SPEC.md. The hooks
    // (p.zoomPunchIn / p.freezeOnImpact) gate those effect modules.
}

void InstantReplayDirector::playReplay(const ClipRecord& clip, ReplayStyle style) {
    bool expected = false;
    if (!playing_.compare_exchange_strong(expected, true)) {
        HC_INFO("Replay already playing - skipping (failsafe: no replay loops)");
        return;   // never stack replays
    }
    // Remember the current (live) scene so we can return to it.
    obs_source_t* cur = obs_frontend_get_current_scene();
    if (cur) { liveSceneName_ = obs_source_get_name(cur); obs_source_release(cur); }

    applyStyle(style, clip);

    obs_frontend_set_current_scene((obs_source_t*)replayScene_);
    HC_INFO("INSTANT REPLAY (%s): %s", presetFor(style).displayName, clip.title.c_str());
}

void InstantReplayDirector::returnToLive() {
    if (!liveSceneName_.empty()) {
        obs_source_t* live = obs_get_source_by_name(liveSceneName_.c_str());
        if (live) { obs_frontend_set_current_scene(live); obs_source_release(live); }
    }
    playing_.store(false);
}

void InstantReplayDirector::mediaEnded(void* data, calldata_t* /*cd*/) {
    auto* self = static_cast<InstantReplayDirector*>(data);
    obs_queue_task(OBS_TASK_UI, [](void* d){
        static_cast<InstantReplayDirector*>(d)->returnToLive();
    }, self, false);
}

#else  // non-OBS host build

void InstantReplayDirector::buildSceneIfNeeded() {}
void InstantReplayDirector::applyStyle(ReplayStyle, const ClipRecord&) {}
void InstantReplayDirector::returnToLive() { playing_.store(false); }
void InstantReplayDirector::playReplay(const ClipRecord& clip, ReplayStyle) {
    HC_INFO("(host stub) would play replay: %s", clip.title.c_str());
}

#endif

} // namespace hypeclip
