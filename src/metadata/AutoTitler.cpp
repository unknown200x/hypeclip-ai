#include "metadata/AutoTitler.hpp"
#include <ctime>
#include <cstdio>

namespace hypeclip {

std::string AutoTitler::clockString(TimePoint) {
    // Use wall clock for the title (steady_clock can't format to time-of-day).
    std::time_t t = std::time(nullptr);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    int hour = lt.tm_hour % 12; if (hour == 0) hour = 12;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d %s", hour, lt.tm_min, lt.tm_hour < 12 ? "AM" : "PM");
    return buf;
}

std::string AutoTitler::adjective(const HighlightEvent& e) {
    if (e.confidence >= 95) return "Unbelievable";
    if (e.confidence >= 88) return "Insane";
    if (e.confidence >= 80) return "Sick";
    return "Nice";
}

std::string AutoTitler::make(const HighlightEvent& e, const std::string& game) {
    std::string core;
    switch (e.type) {
        case HighlightType::Ace:              core = "ACE"; break;
        case HighlightType::TeamWipe:         core = "Team Wipe"; break;
        case HighlightType::MultiKill:        core = "Multi Kill"; break;
        case HighlightType::KillStreak:       core = "Kill Streak"; break;
        case HighlightType::Clutch:           core = adjective(e) + " Clutch"; break;
        case HighlightType::Victory:          core = "Victory Moment"; break;
        case HighlightType::MatchWinningPlay: core = "Match Winning Play"; break;
        case HighlightType::FunnyMoment:      core = "Funny Moment"; break;
        case HighlightType::RageMoment:       core = "Rage Moment"; break;
        case HighlightType::Reaction:         core = adjective(e) + " Reaction"; break;
        case HighlightType::Kill:             core = "Big Play"; break;
        default:                              core = "Highlight"; break;
    }
    if (!game.empty()) return core + " - " + game;
    return core + " - " + clockString(e.triggeredAt);
}

} // namespace hypeclip
