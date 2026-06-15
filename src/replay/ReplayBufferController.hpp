#pragma once
// Wraps the native OBS Replay Buffer (obs_frontend_*). Responsibilities:
//   * detect whether the Replay Buffer is running
//   * (optionally) auto-start it so the plugin works out of the box
//   * trigger a save when a highlight fires
//   * surface the saved file path back to the ClipManager
//
// All frontend calls are marshalled to the OBS UI thread where required.
#include <functional>
#include <string>
#include <atomic>

namespace hypeclip {

class ReplayBufferController {
public:
    using SavedCallback = std::function<void(const std::string& path)>;

    void init();      // registers the OBS frontend event handler
    void shutdown();

    bool isActive() const;
    bool ensureActive();          // starts the buffer if it isn't running
    bool requestSave();           // asks OBS to flush the buffer to disk

    // Invoked (on the OBS thread) when OBS reports REPLAY_BUFFER_SAVED.
    void setSavedCallback(SavedCallback cb) { onSaved_ = std::move(cb); }

    bool autoStartRequested() const { return wantAutoStart_; }
    void setAutoStart(bool v) { wantAutoStart_ = v; }

#if defined(HYPECLIP_FRONTEND)
    static void frontendEvent(enum obs_frontend_event event, void* data);
#endif

private:
    std::string lastSavedPath() const;
    SavedCallback onSaved_;
    std::atomic<bool> wantAutoStart_{true};
};

} // namespace hypeclip
