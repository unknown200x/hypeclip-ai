#include "storage/StorageManager.hpp"
#include "core/EventBus.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

namespace hypeclip {

void StorageManager::start() {
    busToken_ = EventBus::instance().onClipSaved([this](const ClipRecord& r){ addSessionClip(r); });
    runRetention();
}

void StorageManager::stop() {
    if (busToken_) { EventBus::instance().unsubscribe(busToken_); busToken_ = 0; }
}

void StorageManager::addSessionClip(const ClipRecord& r) { session_.push_back(r); }

void StorageManager::runRetention() {
    const Settings s = Config::instance().get();
    if (s.retentionDays <= 0 || s.clipDirectory.empty()) return;
    const auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * s.retentionDays);
    try {
        for (auto& e : fs::recursive_directory_iterator(s.clipDirectory)) {
            if (!e.is_regular_file()) continue;
            auto ext = e.path().extension().string();
            if (ext != ".mkv" && ext != ".mp4" && ext != ".flv") continue;
            auto ftime = fs::last_write_time(e);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            if (sctp < cutoff) {
                std::error_code ec; fs::remove(e.path(), ec);
                if (!ec) HC_INFO("Retention removed %s", e.path().string().c_str());
            }
        }
    } catch (const std::exception& ex) {
        HC_WARN("runRetention failed: %s", ex.what());
    }
}

void StorageManager::enforceSizeCap(uint64_t maxBytes) {
    const Settings s = Config::instance().get();
    if (s.clipDirectory.empty()) return;
    try {
        struct Item { fs::path p; uint64_t size; fs::file_time_type t; };
        std::vector<Item> items; uint64_t total = 0;
        for (auto& e : fs::recursive_directory_iterator(s.clipDirectory)) {
            if (!e.is_regular_file()) continue;
            uint64_t sz = e.file_size();
            total += sz;
            items.push_back({e.path(), sz, fs::last_write_time(e)});
        }
        if (total <= maxBytes) return;
        // Evict oldest first until under cap (flood guard).
        std::sort(items.begin(), items.end(), [](auto&a, auto&b){ return a.t < b.t; });
        for (auto& it : items) {
            if (total <= maxBytes) break;
            std::error_code ec; fs::remove(it.p, ec);
            if (!ec) { total -= it.size; HC_INFO("Size-cap evicted %s", it.p.string().c_str()); }
        }
    } catch (const std::exception& ex) {
        HC_WARN("enforceSizeCap failed: %s", ex.what());
    }
}

std::vector<ClipRecord> StorageManager::bestOf(size_t n) const {
    std::vector<ClipRecord> ranked = session_;
    std::sort(ranked.begin(), ranked.end(),
              [](const ClipRecord& a, const ClipRecord& b){ return a.confidence > b.confidence; });
    if (ranked.size() > n) ranked.resize(n);
    return ranked;
}

std::string StorageManager::writeBestOfPlaylist(size_t n) const {
    const Settings s = Config::instance().get();
    if (s.clipDirectory.empty()) return {};
    auto top = bestOf(n);
    if (top.empty()) return {};
    try {
        fs::path out = fs::path(s.clipDirectory) /
                       ("HypeClip_Top" + std::to_string(n) + ".txt");
        std::ofstream f(out, std::ios::trunc);
        f << "ffconcat version 1.0\n";
        for (const auto& c : top) {
            // ffconcat requires escaped single quotes; clip paths are our own.
            f << "file '" << c.filePath << "'\n";
        }
        HC_INFO("Wrote best-of playlist: %s (%zu clips)", out.string().c_str(), top.size());
        // Optional render (documented): ffmpeg -f concat -safe 0 -i <out> -c copy reel.mp4
        return out.string();
    } catch (const std::exception& ex) {
        HC_WARN("writeBestOfPlaylist failed: %s", ex.what());
        return {};
    }
}

} // namespace hypeclip
