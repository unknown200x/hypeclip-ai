#include "core/PipelineController.hpp"
#include "core/MetricsHub.hpp"
#include "core/Log.hpp"

namespace hypeclip {

PipelineController& PipelineController::instance() { static PipelineController c; return c; }

void PipelineController::startup() {
    if (started_) return;
    started_ = true;

    rb_.init();
    rb_.setAutoStart(true);
    director_.init();
    clipMgr_ = std::make_unique<ClipManager>(rb_, director_);
    clipMgr_->start();
    storage_.start();
    score_.start();

    reconfigure();   // brings up audio/vision according to master + feature flags
    HC_INFO("HypeClip pipeline started");
}

void PipelineController::startAudio(const Settings& s) {
    if (audioRunning_) return;
    auto mk = [](std::vector<std::unique_ptr<AudioCapture>>& vec, CaptureRole role,
                 const std::vector<std::string>& names) {
        if (names.empty()) { vec.push_back(std::make_unique<AudioCapture>(role)); vec.back()->attach(""); }
        else for (const auto& n : names) { vec.push_back(std::make_unique<AudioCapture>(role)); vec.back()->attach(n); }
        for (auto& c : vec) c->start();
    };
    if (s.features.audioDetection && s.features.micDetection)       mk(mics_,  CaptureRole::Mic,  s.micSources);
    if (s.features.audioDetection && s.features.gameAudioDetection) mk(games_, CaptureRole::Game, s.gameSources);
    audioRunning_ = true;
    rb_.ensureActive();
}

void PipelineController::stopAudio() {
    for (auto& c : mics_)  { c->stop(); c->detach(); }
    for (auto& c : games_) { c->stop(); c->detach(); }
    mics_.clear(); games_.clear();
    audioRunning_ = false;
    MetricsHub::instance().reset();
}

void PipelineController::reconfigure() {
    if (!started_) return;
    const Settings s = Config::instance().get();
    score_.reloadConfig();

    // Priority #1: master OFF => full idle (no threads, no resources).
    if (!s.masterEnabled) {
        stopAudio();
        vision_.stop();
        HC_INFO("Master disabled — pipeline idle");
        return;
    }

    // Rebuild audio to reflect device/toggle changes.
    stopAudio();
    startAudio(s);

    // Vision (optional GPU) follows its toggle.
    const bool wantVision = s.features.visualDetection;
    if (wantVision && !vision_.isRunning())      { VisionConfig vc; vc.useOnnx = true; vision_.start(vc); }
    else if (!wantVision && vision_.isRunning()) { vision_.stop(); }

    HC_INFO("Reconfigured (mic=%zu game=%zu vision=%d)", mics_.size(), games_.size(), (int)wantVision);
}

void PipelineController::shutdown() {
    if (!started_) return;
    started_ = false;
    stopAudio();
    vision_.stop();
    score_.stop();
    storage_.stop();
    if (clipMgr_) { clipMgr_->stop(); clipMgr_.reset(); }
    director_.shutdown();
    rb_.shutdown();
    HC_INFO("HypeClip pipeline stopped");
}

} // namespace hypeclip
