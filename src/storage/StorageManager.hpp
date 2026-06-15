#pragma once
// Storage hygiene + end-of-stream reels.
//   * Retention: delete clips older than N days (0 = keep forever).
//   * Flood guard: cap total clip-folder size; evict lowest-confidence first.
//   * Best-of: at stream end produce Top 5 / 10 / 25 ranked by confidence and
//     write an ffconcat playlist (+ optional ffmpeg render if available).
#include "core/Types.hpp"
#include <string>
#include <vector>

namespace hypeclip {

class StorageManager {
public:
    void start();    // subscribe to clip events + STREAMING_STOPPED
    void stop();

    // Housekeeping; safe to call periodically.
    void runRetention();
    void enforceSizeCap(uint64_t maxBytes);

    // Returns the top-N clips of the session ranked by confidence.
    std::vector<ClipRecord> bestOf(size_t n) const;

    // Writes an ffconcat v1 playlist of the top-N to <dir>/HypeClip_Top<N>.txt
    // and returns its path. (Render to a single mp4 is a documented optional
    // step that shells out to ffmpeg when present — see SPEC.md.)
    std::string writeBestOfPlaylist(size_t n) const;

    void addSessionClip(const ClipRecord& r);

private:
    std::vector<ClipRecord> session_;
    uint64_t busToken_ = 0;
};

} // namespace hypeclip
