#include "vision/GameProfile.hpp"

namespace hypeclip {

GameProfile GameProfile::generic() {
    GameProfile p;
    p.name = "";
    // Game-agnostic heuristics: kill-feed churn (lower-right) and full-screen
    // victory bursts (centre band). Conservative scores so audio still leads.
    p.rules = {
        {"killfeed", {0.62f, 0.10f, 0.36f, 0.22f}, HighlightType::Kill,    20.0f},
        {"victory",  {0.20f, 0.30f, 0.60f, 0.40f}, HighlightType::Victory, 45.0f},
    };
    return p;
}

GameProfile GameProfile::loadFromJson(const std::string& /*path*/) {
    // Real implementation parses JSON (regions/rules/onnx). Omitted to avoid a
    // JSON dependency in the scaffold; profiles in data/profiles/*.json are the
    // shipping mechanism. Falls back to generic on any error.
    return generic();
}

std::vector<GameProfile> GameProfile::builtins() {
    std::vector<GameProfile> v;

    auto mk = [](const char* name, std::vector<std::string> procs,
                 std::vector<DetectionRule> rules) {
        GameProfile p; p.name = name; p.processNames = std::move(procs);
        p.rules = std::move(rules); return p;
    };

    v.push_back(mk("Valorant", {"VALORANT-Win64-Shipping.exe"}, {
        {"killfeed", {0.70f, 0.08f, 0.29f, 0.20f}, HighlightType::Kill,    20.f},
        {"spike",    {0.35f, 0.00f, 0.30f, 0.10f}, HighlightType::Clutch,  35.f},
        {"ace",      {0.30f, 0.40f, 0.40f, 0.20f}, HighlightType::Ace,     80.f},
        {"victory",  {0.20f, 0.20f, 0.60f, 0.30f}, HighlightType::Victory, 50.f},
    }));
    v.push_back(mk("Apex Legends", {"r5apex.exe"}, {
        {"knockdown",{0.35f, 0.55f, 0.30f, 0.12f}, HighlightType::Kill,    22.f},
        {"kill",     {0.35f, 0.62f, 0.30f, 0.10f}, HighlightType::Kill,    20.f},
        {"victory",  {0.10f, 0.05f, 0.80f, 0.25f}, HighlightType::Victory, 60.f},
    }));
    v.push_back(mk("Counter-Strike 2", {"cs2.exe"}, {
        {"killfeed", {0.74f, 0.06f, 0.25f, 0.22f}, HighlightType::Kill,     20.f},
        {"roundwin", {0.30f, 0.10f, 0.40f, 0.12f}, HighlightType::Victory,  45.f},
        {"ace",      {0.30f, 0.40f, 0.40f, 0.20f}, HighlightType::Ace,      80.f},
    }));
    v.push_back(mk("Fortnite", {"FortniteClient-Win64-Shipping.exe"}, {
        {"elim",     {0.35f, 0.10f, 0.30f, 0.10f}, HighlightType::Kill,     20.f},
        {"victory",  {0.15f, 0.20f, 0.70f, 0.30f}, HighlightType::Victory,  60.f},
    }));
    v.push_back(mk("Call of Duty", {"cod.exe", "ModernWarfare.exe"}, {
        {"killfeed", {0.70f, 0.08f, 0.29f, 0.22f}, HighlightType::Kill,     20.f},
        {"multikill",{0.30f, 0.30f, 0.40f, 0.20f}, HighlightType::MultiKill,55.f},
    }));
    v.push_back(mk("Overwatch 2", {"Overwatch.exe"}, {
        {"killfeed", {0.72f, 0.08f, 0.27f, 0.22f}, HighlightType::Kill,     20.f},
        {"teamwipe", {0.25f, 0.35f, 0.50f, 0.20f}, HighlightType::TeamWipe, 75.f},
        {"victory",  {0.20f, 0.20f, 0.60f, 0.30f}, HighlightType::Victory,  55.f},
    }));
    v.push_back(mk("League of Legends", {"League of Legends.exe"}, {
        {"multikill",{0.30f, 0.25f, 0.40f, 0.20f}, HighlightType::MultiKill,55.f},
        {"ace",      {0.30f, 0.25f, 0.40f, 0.20f}, HighlightType::Ace,      80.f},
        {"victory",  {0.20f, 0.20f, 0.60f, 0.30f}, HighlightType::Victory,  60.f},
    }));
    v.push_back(mk("Marvel Rivals", {"MarvelRivals.exe", "MarvelGame.exe"}, {
        {"killfeed", {0.70f, 0.08f, 0.29f, 0.20f}, HighlightType::Kill,     20.f},
        {"mvp",      {0.25f, 0.20f, 0.50f, 0.30f}, HighlightType::Victory,  55.f},
    }));
    return v;
}

} // namespace hypeclip
