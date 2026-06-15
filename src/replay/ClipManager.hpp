#pragma once
// Orchestrates clip creation end-to-end:
//   HighlightEvent -> request Replay Buffer save -> receive saved path ->
//   title + tag -> move/organise file -> persist index -> publish ClipRecord
//   -> (if major) hand off to InstantReplayDirector.
//
// Bridges the async nature of OBS replay saves by queueing pending events and
// pairing them with REPLAY_BUFFER_SAVED callbacks FIFO.
#include "core/Types.hpp"
#include "replay/ReplayBufferController.hpp"
#include "replay/InstantReplayDirector.hpp"
#include <deque>
#include <mutex>
#include <vector>
#include <string>

namespace hypeclip {

class ClipManager {
public:
    ClipManager(ReplayBufferController& rb, InstantReplayDirector& director);

    void start();    // subscribe to HighlightEvents
    void stop();

    // Detected game name (set by vision or user). Used for titles/tags/sorting.
    void setCurrentGame(const std::string& g);

    std::vector<ClipRecord> recentClips() const;   // for the UI list

private:
    void onHighlight(const HighlightEvent& e);
    void onReplaySaved(const std::string& srcPath);
    std::string organise(const std::string& srcPath, const ClipRecord& rec) const;
    void persistIndex() const;

    ReplayBufferController& rb_;
    InstantReplayDirector&  director_;

    mutable std::mutex mtx_;
    std::deque<HighlightEvent> pending_;   // awaiting a saved-file path
    std::vector<ClipRecord> clips_;        // session + loaded index
    std::string game_;
    uint64_t busToken_ = 0;
};

} // namespace hypeclip
