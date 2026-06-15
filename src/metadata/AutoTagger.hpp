#pragma once
// Produces metadata tags for a clip (game, type, confidence bucket, modalities).
#include "core/Types.hpp"
#include <string>
#include <vector>

namespace hypeclip {

class AutoTagger {
public:
    static std::vector<std::string> make(const HighlightEvent& e, const std::string& game);
};

} // namespace hypeclip
