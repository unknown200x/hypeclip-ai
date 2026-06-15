#include "core/PipelineController.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"

namespace hypeclip {

PipelineController& PipelineController::instance() {
    static PipelineController c;
    return c;
}

void PipelineController::startup() {
    if (started_) return;
    started_ = true;

    const Settings s = Config::instance().get();

    rb_.init();
    rb_.setAutoStart(true);            // works out of the box: start RB if needed
    director_.init();

    clipMgr_ = std::make_unique<ClipManager>(rb_, director_);
    clipMgr_->start();
    storage_.start();
    score_.start();

    mic_  = std::make_unique<AudioCapture>(CaptureRole::Mic);
    game_ = std::make_unique<AudioCapture>(CaptureRole::Game);
    mic_->attach(s.micSourceName);
    game_->attach(s.gameAudioSourceName);
    mic_->start();
    game_->start();

    if (s.enableVision) {
        VisionConfig vc; vc.useOnnx = true;
        vision_.start(vc);
    }

    // Out-of-the-box safety: make sure the replay buffer is live so the very
    // first highlight can be captured.
    rb_.ensureActive();

    HC_INFO("HypeClip pipeline started (threshold=%.0f, vision=%d)",
            s.threshold, (int)s.enableVision);
}

void PipelineController::reconfigure() {
    if (!started_) return;
    const Settings s = Config::instance().get();

    if (mic_)  { mic_->detach();  mic_->attach(s.micSourceName); }
    if (game_) { game_->detach(); game_->attach(s.gameAudioSourceName); }

    if (s.enableVision && !vision_.isRunning()) {
        VisionConfig vc; vc.useOnnx = true; vision_.start(vc);
    } else if (!s.enableVision && vision_.isRunning()) {
        vision_.stop();
    }
    HC_INFO("HypeClip reconfigured");
}

void PipelineController::shutdown() {
    if (!started_) return;
    started_ = false;

    vision_.stop();
    if (mic_)  { mic_->stop();  mic_->detach();  mic_.reset(); }
    if (game_) { game_->stop(); game_->detach(); game_.reset(); }
    score_.stop();
    storage_.stop();
    if (clipMgr_) { clipMgr_->stop(); clipMgr_.reset(); }
    director_.shutdown();
    rb_.shutdown();
    HC_INFO("HypeClip pipeline stopped");
}

} // namespace hypeclip
