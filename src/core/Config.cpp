#include "core/Config.hpp"
#include "core/Log.hpp"
#include <sstream>

#if defined(HYPECLIP_HAVE_OBS)
#include <util/config-file.h>
#endif

namespace hypeclip {

namespace {
std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> out; std::string cur; std::istringstream ss(s);
    while (std::getline(ss, cur, d)) if (!cur.empty()) out.push_back(cur);
    return out;
}
std::string join(const std::vector<std::string>& v, char d) {
    std::string out; for (size_t i = 0; i < v.size(); ++i) { if (i) out += d; out += v[i]; }
    return out;
}
std::string sanitize(std::string s) { for (char& c : s) if (c=='\n'||c=='|'||c==';'||c==',') c=' '; return s; }
}

Config& Config::instance() { static Config c; return c; }

std::string Config::serializeRules(const std::vector<Rule>& rules) const {
    std::ostringstream os;
    for (const auto& r : rules) {
        os << sanitize(r.name) << '|' << (r.enabled?1:0) << '|' << (int)r.join << '|'
           << (int)r.action << '|' << r.cooldownMs << '|';
        for (size_t i = 0; i < r.conditions.size(); ++i) {
            const auto& c = r.conditions[i];
            if (i) os << ';';
            os << (int)c.metric << ',' << (int)c.op << ',' << c.value;
        }
        os << '\n';
    }
    return os.str();
}

std::vector<Rule> Config::deserializeRules(const std::string& s) const {
    std::vector<Rule> out;
    for (const auto& line : split(s, '\n')) {
        auto f = split(line, '|');
        if (f.size() < 6) continue;
        Rule r;
        r.name = f[0];
        r.enabled = f[1] == "1";
        r.join = (RuleJoin)std::stoi(f[2]);
        r.action = (RuleAction)std::stoi(f[3]);
        r.cooldownMs = std::stoi(f[4]);
        for (const auto& cs : split(f[5], ';')) {
            auto cf = split(cs, ',');
            if (cf.size() < 3) continue;
            RuleCondition c;
            c.metric = (Metric)std::stoi(cf[0]);
            c.op     = (Comparator)std::stoi(cf[1]);
            c.value  = std::stof(cf[2]);
            r.conditions.push_back(c);
        }
        out.push_back(std::move(r));
    }
    return out;
}

void Config::applyBeginnerDefaults() {
    std::lock_guard<std::mutex> lk(mtx_);
    settings_ = Settings{};
    settings_.mode = Mode::Beginner;
    settings_.masterEnabled = true;
    // Beginner: AI scoring on, vision off, native replay buffer (respect user's RB).
}

Settings Config::get() const { std::lock_guard<std::mutex> lk(mtx_); return settings_; }
void Config::set(const Settings& s) { { std::lock_guard<std::mutex> lk(mtx_); settings_ = s; } save(); }

void Config::load(const char* moduleConfigPath) {
    if (moduleConfigPath) path_ = moduleConfigPath;
    applyBeginnerDefaults();
#if defined(HYPECLIP_HAVE_OBS)
    config_t* cfg = nullptr;
    if (config_open(&cfg, path_.c_str(), CONFIG_OPEN_EXISTING) != CONFIG_SUCCESS || !cfg) {
        HC_INFO("No config yet; Beginner defaults"); return;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    auto& s = settings_;
    s.masterEnabled = config_get_bool(cfg, "general", "master_enabled");
    s.mode          = (Mode)config_get_int(cfg, "general", "mode");
    s.threshold     = (float)config_get_double(cfg, "scoring", "threshold");
    s.minClipGapMs  = (int)config_get_int(cfg, "scoring", "min_gap_ms");
    s.retentionDays = (int)config_get_int(cfg, "storage", "retention_days");
    s.falsePositiveLearning = config_get_bool(cfg, "scoring", "fp_learning");

    auto& f = s.features;
    f.autoClip           = config_get_bool(cfg, "features", "auto_clip");
    f.instantReplay      = config_get_bool(cfg, "features", "instant_replay");
    f.visualDetection    = config_get_bool(cfg, "features", "visual");
    f.audioDetection     = config_get_bool(cfg, "features", "audio");
    f.micDetection       = config_get_bool(cfg, "features", "mic");
    f.gameAudioDetection = config_get_bool(cfg, "features", "game_audio");
    f.aiScoring          = config_get_bool(cfg, "features", "ai_scoring");
    f.endOfStreamHighlights = config_get_bool(cfg, "features", "eos_highlights");
    f.autoSocialExport   = config_get_bool(cfg, "features", "social_export");

    auto& r = s.replay;
    r.mode             = (ReplayMode)config_get_int(cfg, "replay", "mode");
    r.preSeconds       = (int)config_get_int(cfg, "replay", "pre");
    r.postSeconds      = (int)config_get_int(cfg, "replay", "post");
    r.replayDurationMs = (int)config_get_int(cfg, "replay", "duration_ms");
    r.enterTransitionMs= (int)config_get_int(cfg, "replay", "enter_ms");
    r.returnTransitionMs=(int)config_get_int(cfg, "replay", "return_ms");
    r.showLabel        = config_get_bool(cfg, "replay", "show_label");
    if (auto* v = config_get_string(cfg, "replay", "scene"))      r.replaySceneName  = v;
    if (auto* v = config_get_string(cfg, "replay", "enter_tr"))   r.enterTransition  = v;
    if (auto* v = config_get_string(cfg, "replay", "return_tr"))  r.returnTransition = v;

    auto& mt = s.micThresholds;
    mt.voiceActivation = (float)config_get_double(cfg, "mic", "voice_act");
    mt.excitement = (float)config_get_double(cfg, "mic", "excitement");
    mt.peak       = (float)config_get_double(cfg, "mic", "peak");
    mt.scream     = (float)config_get_double(cfg, "mic", "scream");
    mt.laughter   = (float)config_get_double(cfg, "mic", "laughter");
    mt.reaction   = (float)config_get_double(cfg, "mic", "reaction");

    auto& gt = s.gameTuning;
    gt.sensitivity      = (float)config_get_double(cfg, "game", "sensitivity");
    gt.eventSensitivity = (float)config_get_double(cfg, "game", "event_sens");
    gt.spikeDetection   = (float)config_get_double(cfg, "game", "spike");
    gt.clustering       = (float)config_get_double(cfg, "game", "clustering");
    gt.cooldownMs       = (int)config_get_int(cfg, "game", "cooldown_ms");

    if (auto* v = config_get_string(cfg, "audio", "mic_sources"))  s.micSources  = split(v, '\n');
    if (auto* v = config_get_string(cfg, "audio", "game_sources")) s.gameSources = split(v, '\n');
    if (auto* v = config_get_string(cfg, "storage", "clip_dir"))   s.clipDirectory = v;
    if (auto* v = config_get_string(cfg, "rules", "data"))         s.rules = deserializeRules(v);
    config_close(cfg);
    HC_INFO("Config v2 loaded (%zu rules, master=%d)", s.rules.size(), (int)s.masterEnabled);
#endif
}

void Config::save() const {
#if defined(HYPECLIP_HAVE_OBS)
    if (path_.empty()) return;
    std::lock_guard<std::mutex> lk(mtx_);
    config_t* cfg = nullptr;
    if (config_open(&cfg, path_.c_str(), CONFIG_OPEN_ALWAYS) != CONFIG_SUCCESS) return;
    const auto& s = settings_;
    config_set_bool(cfg, "general", "master_enabled", s.masterEnabled);
    config_set_int(cfg, "general", "mode", (int)s.mode);
    config_set_double(cfg, "scoring", "threshold", s.threshold);
    config_set_int(cfg, "scoring", "min_gap_ms", s.minClipGapMs);
    config_set_bool(cfg, "scoring", "fp_learning", s.falsePositiveLearning);
    config_set_int(cfg, "storage", "retention_days", s.retentionDays);

    const auto& f = s.features;
    config_set_bool(cfg, "features", "auto_clip", f.autoClip);
    config_set_bool(cfg, "features", "instant_replay", f.instantReplay);
    config_set_bool(cfg, "features", "visual", f.visualDetection);
    config_set_bool(cfg, "features", "audio", f.audioDetection);
    config_set_bool(cfg, "features", "mic", f.micDetection);
    config_set_bool(cfg, "features", "game_audio", f.gameAudioDetection);
    config_set_bool(cfg, "features", "ai_scoring", f.aiScoring);
    config_set_bool(cfg, "features", "eos_highlights", f.endOfStreamHighlights);
    config_set_bool(cfg, "features", "social_export", f.autoSocialExport);

    const auto& r = s.replay;
    config_set_int(cfg, "replay", "mode", (int)r.mode);
    config_set_int(cfg, "replay", "pre", r.preSeconds);
    config_set_int(cfg, "replay", "post", r.postSeconds);
    config_set_int(cfg, "replay", "duration_ms", r.replayDurationMs);
    config_set_int(cfg, "replay", "enter_ms", r.enterTransitionMs);
    config_set_int(cfg, "replay", "return_ms", r.returnTransitionMs);
    config_set_bool(cfg, "replay", "show_label", r.showLabel);
    config_set_string(cfg, "replay", "scene", r.replaySceneName.c_str());
    config_set_string(cfg, "replay", "enter_tr", r.enterTransition.c_str());
    config_set_string(cfg, "replay", "return_tr", r.returnTransition.c_str());

    const auto& mt = s.micThresholds;
    config_set_double(cfg, "mic", "voice_act", mt.voiceActivation);
    config_set_double(cfg, "mic", "excitement", mt.excitement);
    config_set_double(cfg, "mic", "peak", mt.peak);
    config_set_double(cfg, "mic", "scream", mt.scream);
    config_set_double(cfg, "mic", "laughter", mt.laughter);
    config_set_double(cfg, "mic", "reaction", mt.reaction);

    const auto& gt = s.gameTuning;
    config_set_double(cfg, "game", "sensitivity", gt.sensitivity);
    config_set_double(cfg, "game", "event_sens", gt.eventSensitivity);
    config_set_double(cfg, "game", "spike", gt.spikeDetection);
    config_set_double(cfg, "game", "clustering", gt.clustering);
    config_set_int(cfg, "game", "cooldown_ms", gt.cooldownMs);

    config_set_string(cfg, "audio", "mic_sources", join(s.micSources, '\n').c_str());
    config_set_string(cfg, "audio", "game_sources", join(s.gameSources, '\n').c_str());
    config_set_string(cfg, "storage", "clip_dir", s.clipDirectory.c_str());
    config_set_string(cfg, "rules", "data", serializeRules(s.rules).c_str());
    config_save(cfg);
    config_close(cfg);
#endif
}

} // namespace hypeclip
