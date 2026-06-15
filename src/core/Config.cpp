#include "core/Config.hpp"
#include "core/Log.hpp"

#if defined(HYPECLIP_HAVE_OBS)
#include <util/config-file.h>
#endif

namespace hypeclip {

Config& Config::instance() {
    static Config c;
    return c;
}

void Config::applyBeginnerDefaults() {
    std::lock_guard<std::mutex> lk(mtx_);
    settings_ = Settings{};                 // defaults defined in the struct
    settings_.mode = Mode::Beginner;
    settings_.threshold = 70.0f;
    settings_.preSeconds = 20;
    settings_.postSeconds = 10;
    settings_.enableVision = false;         // zero-config: audio-only is safest
}

Settings Config::get() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return settings_;
}

void Config::set(const Settings& s) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        settings_ = s;
    }
    save();
}

void Config::load(const char* moduleConfigPath) {
    if (moduleConfigPath) path_ = moduleConfigPath;
    applyBeginnerDefaults();

#if defined(HYPECLIP_HAVE_OBS)
    config_t* cfg = nullptr;
    if (config_open(&cfg, path_.c_str(), CONFIG_OPEN_EXISTING) == CONFIG_SUCCESS && cfg) {
        std::lock_guard<std::mutex> lk(mtx_);
        settings_.mode = (Mode)config_get_int(cfg, "general", "mode");
        settings_.threshold = (float)config_get_double(cfg, "scoring", "threshold");
        settings_.preSeconds = (int)config_get_int(cfg, "clip", "pre");
        settings_.postSeconds = (int)config_get_int(cfg, "clip", "post");
        settings_.minClipGapMs = (int)config_get_int(cfg, "clip", "min_gap_ms");
        settings_.retentionDays = (int)config_get_int(cfg, "storage", "retention_days");
        settings_.enableVision = config_get_bool(cfg, "vision", "enabled");
        settings_.enableCommentary = config_get_bool(cfg, "replay", "commentary");
        settings_.enableInstantReplay = config_get_bool(cfg, "replay", "instant");
        settings_.replayStyle = (ReplayStyle)config_get_int(cfg, "replay", "style");
        const char* mic = config_get_string(cfg, "audio", "mic_source");
        const char* game = config_get_string(cfg, "audio", "game_source");
        const char* dir = config_get_string(cfg, "storage", "clip_dir");
        if (mic)  settings_.micSourceName = mic;
        if (game) settings_.gameAudioSourceName = game;
        if (dir)  settings_.clipDirectory = dir;
        config_close(cfg);
        HC_INFO("Config loaded from %s", path_.c_str());
    } else {
        HC_INFO("No existing config; using Beginner defaults");
    }
#endif
}

void Config::save() const {
#if defined(HYPECLIP_HAVE_OBS)
    if (path_.empty()) return;
    std::lock_guard<std::mutex> lk(mtx_);
    config_t* cfg = nullptr;
    if (config_open(&cfg, path_.c_str(), CONFIG_OPEN_ALWAYS) != CONFIG_SUCCESS) return;
    config_set_int(cfg, "general", "mode", (int)settings_.mode);
    config_set_double(cfg, "scoring", "threshold", settings_.threshold);
    config_set_int(cfg, "clip", "pre", settings_.preSeconds);
    config_set_int(cfg, "clip", "post", settings_.postSeconds);
    config_set_int(cfg, "clip", "min_gap_ms", settings_.minClipGapMs);
    config_set_int(cfg, "storage", "retention_days", settings_.retentionDays);
    config_set_bool(cfg, "vision", "enabled", settings_.enableVision);
    config_set_bool(cfg, "replay", "commentary", settings_.enableCommentary);
    config_set_bool(cfg, "replay", "instant", settings_.enableInstantReplay);
    config_set_int(cfg, "replay", "style", (int)settings_.replayStyle);
    config_set_string(cfg, "audio", "mic_source", settings_.micSourceName.c_str());
    config_set_string(cfg, "audio", "game_source", settings_.gameAudioSourceName.c_str());
    config_set_string(cfg, "storage", "clip_dir", settings_.clipDirectory.c_str());
    config_save(cfg);
    config_close(cfg);
#endif
}

} // namespace hypeclip
