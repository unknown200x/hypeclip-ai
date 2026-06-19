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

void ClipManager::setCurrentGame(const std::string& g) { std::lock_guard<std::mutex> lk(mtx_); game_ = g; }
std::vector<ClipRecord> ClipManager::recentClips() const { std::lock_guard<std::mutex> lk(mtx_); return clips_; }

void ClipManager::start() {
    rb_.setSavedCallback([this](const std::string& p){ onReplaySaved(p); });
    busToken_ = EventBus::instance().onHighlight([this](const HighlightEvent& e){ onHighlight(e); });
}
void ClipManager::stop() { if (busToken_) { EventBus::instance().unsubscribe(busToken_); busToken_ = 0; } }

void ClipManager::onHighlight(const HighlightEvent& e) {
    const Settings s = Config::instance().get();
    if (!s.masterEnabled || !s.features.autoClip) return;
    if (!rb_.ensureActive()) { HC_WARN("Highlight dropped: replay buffer unavailable"); return; }
    {
        std::lock_guard<std::mutex> lk(mtx_);
        pending_.push_back(e);
        while (pending_.size() > 8) pending_.pop_front();
    }
    rb_.requestSave();   // Native or Custom both flush the RB; length per OBS/RB config
}

void ClipManager::onReplaySaved(const std::string& srcPath) {
    HighlightEvent e; std::string game;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (pending_.empty()) return;
        e = pending_.front(); pending_.pop_front();
        game = game_;
    }
    const Settings s = Config::instance().get();

    ClipRecord rec;
    rec.unixTimeMs  = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch()).count();
    rec.id          = std::to_string(rec.unixTimeMs);
    rec.type        = e.type;
    rec.confidence  = e.confidence;
    rec.game        = game;
    rec.title       = AutoTitler::make(e, game);
    rec.tags        = AutoTagger::make(e, game);
    rec.reason      = e.reason;
    rec.durationSeconds = (s.replay.mode == ReplayMode::CustomClip)
                          ? (s.replay.preSeconds + s.replay.postSeconds) : 0;
    rec.hadReplay   = e.wantReplay;
    rec.filePath    = organise(srcPath, rec);

    { std::lock_guard<std::mutex> lk(mtx_); clips_.push_back(rec); }
    persistIndex();
    EventBus::instance().publishClip(rec);
    HC_INFO("Clip ready: %s (%s)", rec.title.c_str(),
            rec.reason.firedRuleName.empty() ? "AI" : rec.reason.firedRuleName.c_str());

    if (e.wantReplay && s.features.instantReplay) director_.playReplay(rec);
}

std::string ClipManager::organise(const std::string& srcPath, const ClipRecord& rec) const {
    try {
        const Settings s = Config::instance().get();
        fs::path src(srcPath);
        if (!fs::exists(src)) return srcPath;
        fs::path base = s.clipDirectory.empty() ? src.parent_path() / "HypeClip" : fs::path(s.clipDirectory);
        std::string gameDir = rec.game.empty() ? "Unsorted" : rec.game;
        std::time_t t = (std::time_t)(rec.unixTimeMs / 1000); std::tm lt{};
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        char day[16]; std::strftime(day, sizeof(day), "%Y-%m-%d", &lt);
        fs::path dir = base / gameDir / day; fs::create_directories(dir);
        std::string name = rec.title; for (char& c : name) if (c=='/'||c=='\\'||c==':'||c=='*') c='_';
        fs::path dst = dir / (name + src.extension().string());
        int n = 1; while (fs::exists(dst)) dst = dir / (name + "_" + std::to_string(n++) + src.extension().string());
        fs::rename(src, dst);
        return dst.string();
    } catch (const std::exception& ex) { HC_WARN("organise() failed: %s", ex.what()); return srcPath; }
}

// clips.json — matches the DB schema in DESIGN_v2.md (one JSON object per clip).
void ClipManager::persistIndex() const {
    try {
        const Settings s = Config::instance().get();
        if (s.clipDirectory.empty()) return;
        fs::path idx = fs::path(s.clipDirectory) / "clips.json";
        std::ofstream f(idx, std::ios::trunc); if (!f) return;
        std::lock_guard<std::mutex> lk(mtx_);
        f << "[\n";
        for (size_t i = 0; i < clips_.size(); ++i) {
            const auto& c = clips_[i];
            f << "  {\"id\":\"" << c.id << "\",\"title\":\"" << c.title
              << "\",\"file\":\"" << c.filePath << "\",\"game\":\"" << c.game
              << "\",\"type\":\"" << to_string(c.type) << "\",\"confidence\":" << (int)c.confidence
              << ",\"time\":" << c.unixTimeMs << ",\"duration\":" << c.durationSeconds
              << ",\"favorite\":" << (c.favorite ? "true" : "false")
              << ",\"rule\":\"" << c.reason.firedRuleName
              << "\",\"audio\":" << (int)c.reason.audioScore
              << ",\"voice\":" << (int)c.reason.voiceScore
              << ",\"vision\":" << (int)c.reason.visionScore
              << ",\"final\":" << (int)c.reason.finalScore << "}";
            f << (i + 1 < clips_.size() ? ",\n" : "\n");
        }
        f << "]\n";
    } catch (...) {}
}

} // namespace hypeclip
