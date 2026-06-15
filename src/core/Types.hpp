#pragma once
//
// HypeClip AI — shared value types passed between modules over the EventBus.
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
// What kind of signal produced a contribution to the score.
// ---------------------------------------------------------------------------
enum class TriggerSource : uint8_t {
    MicVoice,       // shouting / laughter / vocal spike
    GameAudio,      // explosions, gunfire, victory stings
    Vision,         // kill feed / victory screen / multi-kill banner
    EventHistory,   // derived from recent event density
    Momentum        // derived from escalating intensity
};

// ---------------------------------------------------------------------------
// Semantic classification of a moment (drives titles, tags and replay style).
// ---------------------------------------------------------------------------
enum class HighlightType : uint8_t {
    Generic,
    Kill,
    MultiKill,
    KillStreak,
    Ace,
    TeamWipe,
    Clutch,
    Victory,
    MatchWinningPlay,
    Reaction,      // big vocal reaction with no clear game event
    FunnyMoment,
    RageMoment
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
        default:                          return "?";
    }
}

// A single scored contribution emitted by a detector each analysis tick.
struct Contribution {
    TriggerSource source;
    HighlightType type   = HighlightType::Generic;
    float         score  = 0.0f;   // 0..100 (already weighted by the detector)
    float         energy = 0.0f;   // raw normalized intensity 0..1, for momentum
    TimePoint     when   = Clock::now();
    std::string   label;           // human readable, e.g. "voice spike", "gunfire x3"
};

// The decision object the ScoreEngine emits when threshold is crossed.
struct HighlightEvent {
    float         confidence = 0.0f;   // 0..100, fused multi-layer confidence
    float         rawScore   = 0.0f;   // 0..100, pre-confidence aggregate
    HighlightType type       = HighlightType::Generic;
    TimePoint     triggeredAt = Clock::now();
    std::vector<Contribution> contributors;  // what fed this decision
    bool          majorEvent = false;        // worthy of an instant replay
};

// Persisted record of a clip that was actually written to disk.
struct ClipRecord {
    std::string  filePath;
    std::string  title;
    std::string  game;            // detected or user-set, may be empty
    HighlightType type = HighlightType::Generic;
    float        confidence = 0.0f;
    int64_t      unixTimeMs = 0;
    int          durationSeconds = 0;
    std::vector<std::string> tags;
};

} // namespace hypeclip
