#pragma once
// Central, thread-safe configuration. Backed by an OBS config_t on disk
// (profile/global). Beginner mode ships sane defaults so the plugin "just works".
#include <atomic>
#include <mutex>
#include <string>

namespace hypeclip {

enum class Mode { Beginner, Creator };

enum class ReplayStyle { Esports, Hype, Cinematic, Streamer, Retro };

struct Weights {
    // Per-source multipliers applied to detector scores before aggregation.
    float micVoice    = 1.0f;
    float gameAudio   = 1.0f;
    float vision      = 1.0f;
    float momentum    = 1.0f;
    float eventHistory= 1.0f;
};

struct Settings {
    Mode  mode            = Mode::Beginner;
    float threshold       = 70.0f;   // 0..100; clip when fused confidence >= this
    int   preSeconds      = 20;      // captured before the trigger
    int   postSeconds     = 10;      // captured after the trigger
    int   minClipGapMs    = 8000;    // anti-spam: min spacing between clips
    int   retentionDays   = 30;      // 0 = keep forever

    bool  enableVision    = false;   // off by default for zero-config safety
    bool  enableCommentary= false;
    bool  enableInstantReplay = true;

    ReplayStyle replayStyle = ReplayStyle::Esports;

    std::string micSourceName;       // empty => auto-pick first mic/aux source
    std::string gameAudioSourceName; // empty => desktop audio
    std::string clipDirectory;       // empty => OBS recording output dir

    Weights weights;
};

class Config {
public:
    static Config& instance();

    // Loads from the OBS module config file (no-op safe if OBS absent).
    void load(const char* moduleConfigPath);
    void save() const;

    // Thread-safe snapshot read; cheap enough to call per analysis tick.
    Settings get() const;
    void set(const Settings& s);

    // Applies the immutable Beginner-mode defaults.
    void applyBeginnerDefaults();

private:
    Config() = default;
    mutable std::mutex mtx_;
    Settings settings_;
    std::string path_;
};

} // namespace hypeclip
