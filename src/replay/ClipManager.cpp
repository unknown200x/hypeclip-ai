#include "replay/ClipManager.hpp"
#include "core/EventBus.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "metadata/AutoTitler.hpp"
#include "metadata/AutoTagger.hpp"

#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

namespace hypeclip {

ClipManager::ClipManager(ReplayBufferController& rb, InstantReplayDirector& director)
    : rb_(rb), director_(director) {}

void ClipManager::setCurrentGame(const std::string& g) {
    std::lock_guard<std::mutex> lk(mtx_); game_ = g;
}

std::vector<ClipRecord> ClipManager::recentClips() const {
    std::lock_guard<std::mutex> lk(mtx_); return clips_;
}

void ClipManager::start() {
    rb_.setSavedCallback([this](const std::string& p){ onReplaySaved(p); });
    busToken_ = EventBus::instance().onHighlight([this](const HighlightEvent& e){ onHighlight(e); });
}

void ClipManager::stop() {
    if (busToken_) { EventBus::instance().unsubscribe(busToken_); busToken_ = 0; }
}

void ClipManager::onHighlight(const HighlightEvent& e) {
    // Failsafe: make sure the buffer is live before we count on a save.
    if (!rb_.ensureActive()) {
        HC_WARN("Highlight dropped: replay buffer unavailable");
        return;
    }
    {
        std::lock_guard<std::mutex> lk(mtx_);
        pending_.push_back(e);
        // Bound the queue so a stuck save can't grow memory unbounded.
        while (pending_.size() > 8) pending_.pop_front();
    }
    rb_.requestSave();
}

void ClipManager::onReplaySaved(const std::string& srcPath) {
    HighlightEvent e;
    std::string game;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (pending_.empty()) return;          // a manual/native save, ignore
        e = pending_.front(); pending_.pop_front();
        game = game_;
    }

    ClipRecord rec;
    rec.type        = e.type;
    rec.confidence  = e.confidence;
    rec.game        = game;
    rec.title       = AutoTitler::make(e, game);
    rec.tags        = AutoTagger::make(e, game);
    rec.unixTimeMs  = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch()).count();
    const Settings s = Config::instance().get();
    rec.durationSeconds = s.preSeconds + s.postSeconds;

    rec.filePath = organise(srcPath, rec);

    {
        std::lock_guard<std::mutex> lk(mtx_);
        clips_.push_back(rec);
    }
    persistIndex();
    EventBus::instance().publishClip(rec);
    HC_INFO("Clip ready: %s -> %s", rec.title.c_str(), rec.filePath.c_str());

    if (e.majorEvent && s.enableInstantReplay)
        director_.playReplay(rec, s.replayStyle);
}

// Move the raw replay into <clipDir>/<Game|Unsorted>/<YYYY-MM-DD>/ with a
// descriptive filename. Falls back to leaving it in place on any FS error.
std::string ClipManager::organise(const std::string& srcPath, const ClipRecord& rec) const {
    try {
        const Settings s = Config::instance().get();
        fs::path src(srcPath);
        if (!fs::exists(src)) return srcPath;

        fs::path base = s.clipDirectory.empty() ? src.parent_path() / "HypeClip"
                                                 : fs::path(s.clipDirectory);
        std::string gameDir = rec.game.empty() ? "Unsorted" : rec.game;

        std::time_t t = (std::time_t)(rec.unixTimeMs / 1000);
        std::tm lt{};
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        char day[16]; std::strftime(day, sizeof(day), "%Y-%m-%d", &lt);

        fs::path dir = base / gameDir / day;
        fs::create_directories(dir);

        // Sanitise title into a filename.
        std::string name = rec.title;
        for (char& c : name) if (c == '/' || c == '\\' || c == ':' || c == '*') c = '_';
        fs::path dst = dir / (name + src.extension().string());
        int n = 1;
        while (fs::exists(dst))
            dst = dir / (name + "_" + std::to_string(n++) + src.extension().string());

        fs::rename(src, dst);
        return dst.string();
    } catch (const std::exception& ex) {
        HC_WARN("organise() failed: %s", ex.what());
        return srcPath;
    }
}

// Minimal JSON index alongside clips (hand-rolled to avoid a JSON dependency).
void ClipManager::persistIndex() const {
    try {
        const Settings s = Config::instance().get();
        if (s.clipDirectory.empty()) return;
        fs::path idx = fs::path(s.clipDirectory) / "hypeclip_index.json";
        std::ofstream f(idx, std::ios::trunc);
        if (!f) return;
        std::lock_guard<std::mutex> lk(mtx_);
        f << "[\n";
        for (size_t i = 0; i < clips_.size(); ++i) {
            const auto& c = clips_[i];
            f << "  {\"title\":\"" << c.title << "\",\"file\":\"" << c.filePath
              << "\",\"game\":\"" << c.game << "\",\"type\":\"" << to_string(c.type)
              << "\",\"confidence\":" << (int)c.confidence
              << ",\"time\":" << c.unixTimeMs << "}";
            f << (i + 1 < clips_.size() ? ",\n" : "\n");
        }
        f << "]\n";
    } catch (...) {}
}

} // namespace hypeclip
