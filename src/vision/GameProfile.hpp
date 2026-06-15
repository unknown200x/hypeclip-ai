#pragma once
// Declarative, data-driven game profile. The plugin stays game-agnostic: a
// profile only adds screen-region hints + (optional) an ONNX model so vision
// detection is more precise for popular titles. Profiles ship as JSON in
// data/profiles/<game>.json and are hot-loadable.
#include "core/Types.hpp"
#include <string>
#include <vector>

namespace hypeclip {

struct Region { float x, y, w, h; };   // normalised 0..1 screen rectangle

struct DetectionRule {
    std::string   name;        // "killfeed", "victory", "ace", ...
    Region        region;      // where to look
    HighlightType type;        // what it maps to
    float         baseScore;   // contribution score on positive
};

class GameProfile {
public:
    std::string name;                       // "Valorant", "Apex Legends", ...
    std::vector<std::string> processNames;  // for auto-detection (foreground exe)
    std::vector<DetectionRule> rules;
    std::string onnxModelPath;              // optional classifier

    static GameProfile loadFromJson(const std::string& path);

    // Built-in, code-defined fallbacks for the supported titles. Game-agnostic
    // generic profile is returned when none match.
    static GameProfile generic();
    static std::vector<GameProfile> builtins();   // Valorant, Apex, CS2, ...
};

} // namespace hypeclip
