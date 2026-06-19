#pragma once
// Central, thread-safe configuration (v2). Backed by an OBS config_t on disk.
// Everything the user can control lives here; the whole plugin reads snapshots.
#include "core/Types.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

namespace hypeclip {

enum class Mode { Beginner, Creator };
enum class ReplayMode { NativeBuffer, CustomClip };

// Priority #1 — independent feature toggles.
struct FeatureFlags {
    bool autoClip              = true;
    bool instantReplay         = true;
    bool visualDetection       = false;  // GPU vision, off by default
    bool audioDetection        = true;
    bool micDetection          = true;
    bool gameAudioDetection    = true;
    bool aiScoring             = true;   // the fused-confidence scorer
    bool endOfStreamHighlights = true;
    bool autoSocialExport      = false;
};

// Priority #2/#3/#4 — replay behaviour.
struct ReplayConfig {
    ReplayMode  mode             = ReplayMode::NativeBuffer; // respect user's RB
    int         preSeconds       = 15;   // CustomClip only
    int         postSeconds      = 20;   // CustomClip only
    std::string replaySceneName;         // empty => plugin-owned minimal scene
    std::string enterTransition  = "Fade";
    int         enterTransitionMs= 300;
    int         replayDurationMs = 8000;
    std::string returnTransition = "Fade";
    int         returnTransitionMs = 300;
    bool        showLabel        = true; // the single centered "INSTANT REPLAY"
};

// Priority #5 — independent mic thresholds (0..100).
struct MicThresholds {
    float voiceActivation = 15;
    float excitement      = 60;
    float peak            = 70;
    float scream          = 80;
    float laughter        = 65;
    float reaction        = 55;
};

// Priority #6 — game-audio tuning (0..100 except cooldown).
struct GameAudioTuning {
    float sensitivity      = 50;
    float eventSensitivity = 50;
    float spikeDetection   = 60;
    float clustering       = 50;
    int   cooldownMs       = 600;
};

struct Weights {
    float micVoice = 1.0f, gameAudio = 1.0f, vision = 1.0f,
          momentum = 1.0f, eventHistory = 1.0f;
};

struct Settings {
    bool        masterEnabled = true;          // Priority #1 master switch
    Mode        mode          = Mode::Beginner;
    FeatureFlags features;

    float       threshold     = 70.0f;         // AI-scoring clip threshold
    int         minClipGapMs  = 8000;          // global anti-spam
    int         retentionDays = 30;
    bool        falsePositiveLearning = true;  // Priority #9

    ReplayConfig    replay;
    MicThresholds   micThresholds;
    GameAudioTuning gameTuning;
    Weights         weights;

    std::vector<std::string> micSources;       // multiple (empty => auto)
    std::vector<std::string> gameSources;      // multiple (empty => desktop)
    std::vector<Rule>        rules;            // Priority #7 (empty => AI scoring only)

    std::string clipDirectory;
};

class Config {
public:
    static Config& instance();
    void load(const char* moduleConfigPath);
    void save() const;

    Settings get() const;
    void set(const Settings& s);

    void applyBeginnerDefaults();

private:
    Config() = default;
    std::string serializeRules(const std::vector<Rule>&) const;
    std::vector<Rule> deserializeRules(const std::string&) const;

    mutable std::mutex mtx_;
    Settings settings_;
    std::string path_;
};

} // namespace hypeclip
