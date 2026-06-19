#include "replay/InstantReplayDirector.hpp"
#include "core/Log.hpp"
#include <chrono>

#if defined(HYPECLIP_FRONTEND)
#include <obs-frontend-api.h>
#endif

namespace hypeclip {

static const char* kMinimalScene = "HypeClip Instant Replay";

void InstantReplayDirector::init() {
#if defined(HYPECLIP_FRONTEND)
    buildMinimalSceneIfNeeded();
    HC_INFO("InstantReplayDirector ready (clean mode)");
#endif
}

void InstantReplayDirector::shutdown() {
    if (timer_.joinable()) timer_.join();
#if defined(HYPECLIP_FRONTEND)
    if (mediaSource_)  { obs_source_release((obs_source_t*)mediaSource_);  mediaSource_  = nullptr; }
    if (label_)        { obs_source_release((obs_source_t*)label_);        label_        = nullptr; }
    if (minimalScene_) { obs_source_release((obs_source_t*)minimalScene_); minimalScene_ = nullptr; }
#endif
}

#if defined(HYPECLIP_FRONTEND)

void InstantReplayDirector::buildMinimalSceneIfNeeded() {
    if (obs_source_t* existing = obs_get_source_by_name(kMinimalScene)) { minimalScene_ = existing; }
    else {
        obs_scene_t* scene = obs_scene_create(kMinimalScene);
        minimalScene_ = obs_source_get_ref(obs_scene_get_source(scene));
    }
    obs_scene_t* scene = obs_scene_from_source((obs_source_t*)minimalScene_);

    if (!mediaSource_) {
        obs_data_t* set = obs_data_create();
        obs_data_set_bool(set, "looping", false);
        obs_data_set_bool(set, "restart_on_activate", true);
        mediaSource_ = obs_source_create("ffmpeg_source", "HypeClip Replay Media", set, nullptr);
        obs_data_release(set);
        obs_scene_add(scene, (obs_source_t*)mediaSource_);
    }
    if (!label_) {
        obs_data_t* set = obs_data_create();
        obs_data_set_string(set, "text", "INSTANT REPLAY");
#if defined(_WIN32)
        const char* textId = "text_gdiplus_v2";
#else
        const char* textId = "text_ft2_source_v2";
#endif
        label_ = obs_source_create(textId, "HypeClip Replay Label", set, nullptr);
        obs_data_release(set);
        obs_sceneitem_t* item = obs_scene_add(scene, (obs_source_t*)label_);
        if (item) {
            // Center the single label near the top-middle (broadcast style).
            vec2 pos; pos.x = 0; pos.y = 60;
            obs_sceneitem_set_alignment(item, OBS_ALIGN_TOP | OBS_ALIGN_CENTER);
            obs_sceneitem_set_pos(item, &pos);
        }
    }
}

obs_source_t* InstantReplayDirector::resolveReplayScene(const ReplayConfig& rc) {
    if (!rc.replaySceneName.empty()) {
        if (obs_source_t* user = obs_get_source_by_name(rc.replaySceneName.c_str())) return user;
        HC_WARN("Replay scene '%s' not found; using minimal scene", rc.replaySceneName.c_str());
    }
    // Plugin-owned minimal scene: point media at the clip, label visibility.
    if (mediaSource_) {
        obs_data_t* set = obs_source_get_settings((obs_source_t*)mediaSource_);
        // (clip path filled by caller via playReplay)
        obs_data_release(set);
    }
return obs_source_get_ref((obs_source_t*)minimalScene_);  // caller releases
}

void InstantReplayDirector::applyTransition(const std::string& name, int durationMs) {
    if (name.empty()) return;
    obs_frontend_source_list ts = {};
    obs_frontend_get_transitions(&ts);
    for (size_t i = 0; i < ts.sources.num; ++i) {
        obs_source_t* tr = ts.sources.array[i];
        if (name == obs_source_get_name(tr)) { obs_frontend_set_current_transition(tr); break; }
    }
    obs_frontend_source_list_free(&ts);
    if (durationMs > 0) obs_frontend_set_transition_duration(durationMs);
}

void InstantReplayDirector::playReplay(const ClipRecord& clip) {
    bool expected = false;
    if (!playing_.compare_exchange_strong(expected, true)) {
        HC_INFO("Replay already playing — skipping (no loops)");
        return;
    }
    const ReplayConfig rc = Config::instance().get().replay;

    if (obs_source_t* cur = obs_frontend_get_current_scene()) {
        liveSceneName_ = obs_source_get_name(cur);
        obs_source_release(cur);
    }

    // If using the plugin scene, load the saved clip + label visibility now.
    if (rc.replaySceneName.empty() && mediaSource_) {
        obs_data_t* set = obs_source_get_settings((obs_source_t*)mediaSource_);
        obs_data_set_string(set, "local_file", clip.filePath.c_str());
        obs_source_update((obs_source_t*)mediaSource_, set);
        obs_data_release(set);
        if (label_) obs_source_set_enabled((obs_source_t*)label_, rc.showLabel);
    }

    applyTransition(rc.enterTransition, rc.enterTransitionMs);
    obs_source_t* scene = resolveReplayScene(rc);
    obs_frontend_set_current_scene(scene);
    obs_source_release(scene);
    HC_INFO("INSTANT REPLAY -> %s for %dms",
            rc.replaySceneName.empty() ? kMinimalScene : rc.replaySceneName.c_str(), rc.replayDurationMs);

    if (timer_.joinable()) timer_.join();
    const int holdMs = rc.replayDurationMs;
    timer_ = std::thread([this, holdMs]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(holdMs));
        obs_queue_task(OBS_TASK_UI, [](void* d){ static_cast<InstantReplayDirector*>(d)->returnToLive(); },
                       this, false);
    });
}

void InstantReplayDirector::returnToLive() {
    const ReplayConfig rc = Config::instance().get().replay;
    applyTransition(rc.returnTransition, rc.returnTransitionMs);
    if (!liveSceneName_.empty()) {
        if (obs_source_t* live = obs_get_source_by_name(liveSceneName_.c_str())) {
            obs_frontend_set_current_scene(live);
            obs_source_release(live);
        }
    }
    playing_.store(false);
}

#else  // host build

void InstantReplayDirector::buildMinimalSceneIfNeeded() {}
void InstantReplayDirector::returnToLive() { playing_.store(false); }
void InstantReplayDirector::playReplay(const ClipRecord& clip) {
    HC_INFO("(host stub) replay: %s", clip.title.c_str());
}

#endif

} // namespace hypeclip
