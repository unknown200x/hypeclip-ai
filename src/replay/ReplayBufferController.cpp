#include "replay/ReplayBufferController.hpp"
#include "core/Log.hpp"

#if defined(HYPECLIP_FRONTEND)
#include <obs-frontend-api.h>
#include <obs.h>
#endif

namespace hypeclip {

// Single instance pointer so the C-style frontend callback can reach us.
static ReplayBufferController* g_self = nullptr;

void ReplayBufferController::init() {
    g_self = this;
#if defined(HYPECLIP_FRONTEND)
    obs_frontend_add_event_callback(&ReplayBufferController::frontendEvent, this);
    HC_INFO("ReplayBufferController initialised");
#endif
}

void ReplayBufferController::shutdown() {
#if defined(HYPECLIP_FRONTEND)
    obs_frontend_remove_event_callback(&ReplayBufferController::frontendEvent, this);
#endif
    g_self = nullptr;
}

bool ReplayBufferController::isActive() const {
#if defined(HYPECLIP_FRONTEND)
    return obs_frontend_replay_buffer_active();
#else
    return true;
#endif
}

bool ReplayBufferController::ensureActive() {
#if defined(HYPECLIP_FRONTEND)
    if (obs_frontend_replay_buffer_active()) return true;
    if (!wantAutoStart_) return false;
    HC_INFO("Replay Buffer not active — starting it");
    obs_frontend_replay_buffer_start();
    return obs_frontend_replay_buffer_active();
#else
    return true;
#endif
}

bool ReplayBufferController::requestSave() {
#if defined(HYPECLIP_FRONTEND)
    if (!isActive() && !ensureActive()) {
        HC_WARN("Cannot save: Replay Buffer inactive and auto-start disabled");
        return false;
    }
    obs_frontend_replay_buffer_save();   // async; path arrives via frontendEvent
    return true;
#else
    if (onSaved_) onSaved_("/tmp/hypeclip_test_clip.mkv");
    return true;
#endif
}

std::string ReplayBufferController::lastSavedPath() const {
#if defined(HYPECLIP_FRONTEND)
    // obs_frontend_get_last_replay() returns a strdup'd path (OBS 27+).
    char* p = obs_frontend_get_last_replay();
    std::string out = p ? p : "";
    bfree(p);
    return out;
#else
    return {};
#endif
}

#if defined(HYPECLIP_FRONTEND)
void ReplayBufferController::frontendEvent(enum obs_frontend_event event, void* data) {
    auto* self = static_cast<ReplayBufferController*>(data);
    if (!self) return;
    switch (event) {
        case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED: {
            const std::string path = self->lastSavedPath();
            HC_INFO("Replay saved: %s", path.c_str());
            if (self->onSaved_ && !path.empty()) self->onSaved_(path);
            break;
        }
        case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
            HC_WARN("Replay Buffer stopped");
            break;
        default: break;
    }
}
#endif

} // namespace hypeclip
