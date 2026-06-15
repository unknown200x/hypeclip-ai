#include "metadata/AutoTagger.hpp"

namespace hypeclip {

std::vector<std::string> AutoTagger::make(const HighlightEvent& e, const std::string& game) {
    std::vector<std::string> tags;
    if (!game.empty()) tags.push_back(game);
    tags.emplace_back(to_string(e.type));

    if (e.confidence >= 90)      tags.emplace_back("top-tier");
    else if (e.confidence >= 80) tags.emplace_back("highlight");

    bool hasVoice = false, hasGame = false, hasVision = false;
    for (const auto& c : e.contributors) {
        switch (c.source) {
            case TriggerSource::MicVoice:  hasVoice = true;  break;
            case TriggerSource::GameAudio: hasGame = true;   break;
            case TriggerSource::Vision:    hasVision = true; break;
            default: break;
        }
    }
    if (hasVoice)  tags.emplace_back("reaction");
    if (hasGame)   tags.emplace_back("combat");
    if (hasVision) tags.emplace_back("on-screen");
    if (e.majorEvent) tags.emplace_back("instant-replay");
    return tags;
}

} // namespace hypeclip
