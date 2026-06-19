#include "audio/AudioCapture.hpp"
#include "core/EventBus.hpp"
#include "core/MetricsHub.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"

#include <chrono>
#include <algorithm>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs-module.h>
#endif

namespace hypeclip {

static constexpr size_t kWindow = 960;   // ~20 ms @ 48 kHz

AudioCapture::AudioCapture(CaptureRole role) : role_(role) {
    if (role_ == CaptureRole::Mic) voice_ = std::make_unique<VoiceExcitementDetector>();
    else                           game_  = std::make_unique<GameAudioDetector>();
    scratch_.resize(kWindow);
}
AudioCapture::~AudioCapture() { stop(); detach(); }

#if defined(HYPECLIP_HAVE_OBS)

bool AudioCapture::attach(const std::string& sourceName) {
    detach();
    sourceName_ = sourceName;
    obs_source_t* src = nullptr;
    if (!sourceName.empty()) src = obs_get_source_by_name(sourceName.c_str());
    else                     src = obs_get_output_source(role_ == CaptureRole::Mic ? 3 : 1);
    if (!src) { HC_WARN("AudioCapture: no source for role %d", (int)role_); return false; }
    source_ = src;
    sampleRate_.store((int)audio_output_get_sample_rate(obs_get_audio()));
    extractor_.setSampleRate(sampleRate_.load());
    obs_source_add_audio_capture_callback(source_, &AudioCapture::obsAudioCb, this);
    HC_INFO("AudioCapture attached: %s", obs_source_get_name(source_));
    return true;
}
void AudioCapture::detach() {
    if (source_) {
        obs_source_remove_audio_capture_callback(source_, &AudioCapture::obsAudioCb, this);
        obs_source_release(source_); source_ = nullptr;
    }
}
void AudioCapture::obsAudioCb(void* param, obs_source_t*, const struct audio_data* audio, bool muted) {
    auto* self = static_cast<AudioCapture*>(param);
    if (!self || !audio || audio->frames == 0 || muted) return;
    const int channels = (int)audio_output_get_channels(obs_get_audio());
    downmixToMono(reinterpret_cast<const float* const*>(audio->data), channels, audio->frames, self->downmix_);
    self->ring_.push(self->downmix_.data(), audio->frames);
}

#else
bool AudioCapture::attach(const std::string& n) { sourceName_ = n; return true; }
void AudioCapture::detach() {}
#endif

void AudioCapture::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&AudioCapture::analysisLoop, this);
}
void AudioCapture::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

void AudioCapture::analysisLoop() {
    auto& bus = EventBus::instance();
    auto& hub = MetricsHub::instance();
    while (running_.load(std::memory_order_relaxed)) {
        if (ring_.size() < kWindow) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); continue; }
        ring_.pop(scratch_.data(), kWindow);
        FrameFeatures ff = extractor_.analyze(scratch_.data(), kWindow);
        const auto now = Clock::now();
        const Settings s = Config::instance().get();
        if (!s.masterEnabled || !s.features.audioDetection) continue;

        if (role_ == CaptureRole::Mic) {
            if (!s.features.micDetection) continue;
            MicScores m = voice_->analyze(ff, now);
            hub.updateMic(m.level, m.excitement, m.peak, m.scream, m.laughter, m.reaction);
            meter_.store(m.excitement / 100.0f);

            const auto& t = s.micThresholds;
            if (m.level < t.voiceActivation) continue;          // voice gate
            if (now - lastEmit_ < Millis(500)) continue;

            float score = 0; HighlightType type = HighlightType::Reaction; const char* label = "";
            if (m.scream >= t.scream)            { score = 45; label = "mic scream"; }
            else if (m.reaction >= t.reaction)   { score = 32; label = "sustained reaction"; }
            else if (m.laughter >= t.laughter)   { score = 30; type = HighlightType::FunnyMoment; label = "laughter"; }
            else if (m.peak >= t.peak)           { score = 25; label = "vocal peak"; }
            else if (m.excitement >= t.excitement){ score = 22; label = "voice spike"; }
            else continue;

            lastEmit_ = now;
            Contribution c; c.source = TriggerSource::MicVoice; c.type = type;
            c.score = score; c.energy = m.excitement / 100.0f; c.when = now; c.label = label;
            bus.publish(c);
        } else {
            if (!s.features.gameAudioDetection) continue;
            const auto& g = s.gameTuning;
            GameScores gs = game_->analyze(ff, g.spikeDetection, g.clustering);
            hub.updateGame(gs.level, gs.spike, gs.intensity);
            meter_.store(gs.intensity / 100.0f);

            if (now - lastEmit_ < Millis(g.cooldownMs)) continue;
            const int combatRun = 6 - (int)(g.eventSensitivity / 100.0f * 4.0f);  // 6..2
            float score = 0; HighlightType type = HighlightType::Generic; const char* label = "";
            if (gs.musicalStinger)            { score = 50; type = HighlightType::Victory; label = "victory stinger"; }
            else if (gs.transientRun >= combatRun){ score = 25; type = HighlightType::Kill; label = "sustained combat"; }
            else continue;

            lastEmit_ = now;
            Contribution c; c.source = TriggerSource::GameAudio; c.type = type;
            c.score = score; c.energy = gs.intensity / 100.0f; c.when = now; c.label = label;
            bus.publish(c);
        }
    }
}

} // namespace hypeclip
