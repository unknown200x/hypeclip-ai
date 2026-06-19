#pragma once
//
// HypeClip AI — shared value types (v2 redesign).
// Header-only, POD-ish, no OBS dependency so they are unit-testable on any host.
//
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace hypeclip {

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Millis    = std::chrono::milliseconds;

// ---------------------------------------------------------------------------
enum class TriggerSource : uint8_t {
    MicVoice, GameAudio, Vision, EventHistory, Momentum, Rule
};

enum class HighlightType : uint8_t {
    Generic, Kill, MultiKill, KillStreak, Ace, TeamWipe, Clutch,
    Victory, MatchWinningPlay, Reaction, FunnyMoment, RageMoment
};

inline const char* to_string(HighlightType t) noexcept {
    switch (t) {
        case HighlightType::Kill:             return "Kill";
        case HighlightType::MultiKill:        return "Multi-Kill";
        case HighlightType::KillStreak:       return "Kill Streak";
        case HighlightType::Ace:              return "Ace";
        case HighlightType::TeamWipe:         return "Team Wipe";
        case HighlightType::Clutch:           return "Clutch";
        case HighlightType::Victory:          return "Victory";
        case HighlightType::MatchWinningPlay: return "Match-Winning Play";
        case HighlightType::Reaction:         return "Reaction";
        case HighlightType::FunnyMoment:      return "Funny Moment";
        case HighlightType::RageMoment:       return "Rage Moment";
        default:                              return "Highlight";
    }
}
inline const char* to_string(TriggerSource s) noexcept {
    switch (s) {
        case TriggerSource::MicVoice:     return "mic";
        case TriggerSource::GameAudio:    return "game-audio";
        case TriggerSource::Vision:       return "vision";
        case TriggerSource::EventHistory: return "history";
        case TriggerSource::Momentum:     return "momentum";
        case TriggerSource::Rule:         return "rule";
        default:                          return "?";
    }
}

// ---------------------------------------------------------------------------
// Live metric snapshot — the single source of truth the UI meters, the rule
// engine and the AI scorer all read. Every value normalised 0..100 unless noted.
// ---------------------------------------------------------------------------
struct MetricSnapshot {
    float micLevel       = 0;   // instantaneous mic loudness 0..100
    float micExcitement  = 0;   // how far above the speaker's baseline 0..100
    float micPeak        = 0;   // short-term peak 0..100
    float screamScore    = 0;   // vocal-strain / scream likelihood 0..100
    float laughterScore  = 0;   // rhythmic-voiced-burst likelihood 0..100
    float reactionScore  = 0;   // sustained reaction energy 0..100

    float gameLevel      = 0;   // game-audio loudness 0..100
    float gameSpike      = 0;   // transient/onset strength 0..100
    float gameIntensity  = 0;   // smoothed combat intensity 0..100

    float visionScore    = 0;   // on-screen event confidence 0..100
    bool  killFeed       = false;

    float momentum       = 0;   // 0..100 escalation
    float confidence     = 0;   // fused AI confidence 0..100
    float clipWorthiness = 0;   // max(confidence, best rule progress) 0..100
};

// A single scored contribution emitted by a detector each analysis tick.
struct Contribution {
    TriggerSource source;
    HighlightType type   = HighlightType::Generic;
    float         score  = 0.0f;
    float         energy = 0.0f;
    TimePoint     when   = Clock::now();
    std::string   label;
};

// Transparent breakdown of WHY a clip fired (Priority #8).
struct ScoreReason {
    float audioScore   = 0;
    float voiceScore   = 0;
    float visionScore  = 0;
    float momentum     = 0;
    float finalScore   = 0;     // == confidence at fire time
    TriggerSource primarySource = TriggerSource::MicVoice;
    std::string   firedRuleName;   // empty if AI-scoring path fired it
    std::string   explanation;     // human-readable one-liner
};

struct HighlightEvent {
    float         confidence = 0.0f;
    float         rawScore   = 0.0f;
    HighlightType type       = HighlightType::Generic;
    TimePoint     triggeredAt = Clock::now();
    std::vector<Contribution> contributors;
    ScoreReason   reason;
    bool          majorEvent = false;   // worthy of an instant replay
    bool          wantReplay = false;   // action requested a replay specifically
};

// Persisted clip record (maps 1:1 to the clips DB row — see DESIGN_v2.md).
struct ClipRecord {
    std::string  id;              // uuid
    std::string  filePath;
    std::string  title;
    std::string  game;
    HighlightType type = HighlightType::Generic;
    float        confidence = 0.0f;
    int64_t      unixTimeMs = 0;
    int          durationSeconds = 0;
    std::vector<std::string> tags;
    ScoreReason  reason;
    bool         favorite = false;
    bool         hadReplay = false;
};

// ---------------------------------------------------------------------------
// Rule engine value types (Priority #7).
// ---------------------------------------------------------------------------
enum class Metric : uint8_t {
    MicExcitement, MicVolume, MicPeak, ScreamScore, LaughterScore, ReactionScore,
    GameSpike, GameIntensity, GameLevel, VisionScore, KillFeed, Momentum, Confidence
};

enum class Comparator : uint8_t { Greater, GreaterEqual, Less, LessEqual };
enum class RuleAction : uint8_t { CreateClip, CreateReplay };
enum class RuleJoin   : uint8_t { All /*AND*/, Any /*OR*/ };

inline const char* to_string(Metric m) noexcept {
    switch (m) {
        case Metric::MicExcitement: return "Mic Excitement";
        case Metric::MicVolume:     return "Mic Volume";
        case Metric::MicPeak:       return "Mic Peak";
        case Metric::ScreamScore:   return "Scream";
        case Metric::LaughterScore: return "Laughter";
        case Metric::ReactionScore: return "Reaction";
        case Metric::GameSpike:     return "Game Audio Spike";
        case Metric::GameIntensity: return "Game Intensity";
        case Metric::GameLevel:     return "Game Volume";
        case Metric::VisionScore:   return "Vision Score";
        case Metric::KillFeed:      return "Kill Feed Detected";
        case Metric::Momentum:      return "Momentum";
        case Metric::Confidence:    return "AI Confidence";
        default:                    return "?";
    }
}
inline const char* to_string(Comparator c) noexcept {
    switch (c) { case Comparator::Greater: return ">"; case Comparator::GreaterEqual: return ">=";
                 case Comparator::Less: return "<"; default: return "<="; }
}

struct RuleCondition {
    Metric     metric = Metric::MicExcitement;
    Comparator op     = Comparator::Greater;
    float      value  = 80.0f;
};

struct Rule {
    std::string name = "New Rule";
    bool        enabled = true;
    RuleJoin    join = RuleJoin::All;
    std::vector<RuleCondition> conditions;
    RuleAction  action = RuleAction::CreateClip;
    int         cooldownMs = 8000;   // per-rule anti-spam
};

} // namespace hypeclip
