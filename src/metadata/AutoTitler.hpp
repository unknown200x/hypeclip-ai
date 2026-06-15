#pragma once
// Generates human-friendly clip titles from a HighlightEvent.
//   "Triple Kill - 8:14 PM", "Insane Clutch - Valorant", "Match Winning Play"
#include "core/Types.hpp"
#include <string>

namespace hypeclip {

class AutoTitler {
public:
    // game may be empty (game-agnostic). Local 12h time is appended for context.
    static std::string make(const HighlightEvent& e, const std::string& game);
private:
    static std::string adjective(const HighlightEvent& e);
    static std::string clockString(TimePoint when);
};

} // namespace hypeclip
