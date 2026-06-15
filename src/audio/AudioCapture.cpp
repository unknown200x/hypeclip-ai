#include "audio/AudioCapture.hpp"
#include "core/EventBus.hpp"
#include "core/Log.hpp"

#include <chrono>
#include <cmath>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs.h>
#include <obs-module.h>
#endif

namespace hypeclip {

// Analysis window ≈ 20 ms at 48 kHz => 960 samples; we read in these chunks.
static constexpr size_t kWindow = 960;

AudioCapture::AudioCapture(CaptureRole role) : role_(role) {
    if (role_ == CaptureRole::Mic)  voice_ = std::make_unique<VoiceExcitementDetector>();
    else                            game_  = std::make_unique<GameAudioDetector>();
    scratch_.resize(kWindow);
}

AudioCapture::~AudioCapture() { stop(); detach(); }

float AudioCapture::meterLevel() const { return meter_.load(std::memory_order_relaxed); }

#if defined(HYPECLIP_HAVE_OBS)

bool AudioCapture::attach(const std::string& sourceName) {
    detach();
    sourceName_ = sourceName;

    obs_source_t* src = nullptr;
    if (!sourceName.empty()) {
        src = obs_get_source_by_name(sourceName.c_str());
    } else {
        // Auto-pick: mic => first available aux/mic global audio channel,
        // game => desktop audio channel. OBS exposes these on channels 1..6.
        const int channel = (role_ == CaptureRole::Mic) ? 3 : 1;
        src = obs_get_output_source(channel);
    }
    if (!src) { HC_WARN("AudioCapture: no source for role %d", (int)role_); return false; }

    source_ = src;
    sampleRate_.store((int)audio_output_get_sample_rate(obs_get_audio()));
    extractor_.setSampleRate(sampleRate_.load());
    obs_source_add_audio_capture_callback(source_, &AudioCapture::obsAudioCb, this);
    HC_INFO("AudioCapture attached (%s) @%d Hz",
            obs_source_get_name(source_), sampleRate_.load());
    return true;
}

void AudioCapture::detach() {
    if (source_) {
        obs_source_remove_audio_capture_callback(source_, &AudioCapture::obsAudioCb, this);
        obs_source_release(source_);
        source_ = nullptr;
    }
}

// Runs on the OBS audio thread. Keep it tiny: downmix + push, nothing else.
void AudioCapture::obsAudioCb(void* param, obs_source_t*,
                              const struct audio_data* audio, bool muted) {
    auto* self = static_cast<AudioCapture*>(param);
    if (!self || !audio || audio->frames == 0) return;
    if (muted) return;  // muted mic should not trigger anything

    const int channels = (int)audio_output_get_channels(obs_get_audio());
    downmixToMono(reinterpret_cast<const float* const*>(audio->data),
                  channels, audio->frames, self->downmix_);
    self->ring_.push(self->downmix_.data(), audio->frames);
}

#else  // host/unit-test build without OBS

bool AudioCapture::attach(const std::string& name) { sourceName_ = name; return true; }
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
    while (running_.load(std::memory_order_relaxed)) {
        if (ring_.size() >= kWindow) {
            ring_.pop(scratch_.data(), kWindow);
            FrameFeatures ff = extractor_.analyze(scratch_.data(), kWindow);
            const auto now = Clock::now();

            std::optional<Contribution> c;
            if (role_ == CaptureRole::Mic) {
                c = voice_->feed(ff, now);
                meter_.store(voice_->excitement());
            } else {
                c = game_->feed(ff, now);
                meter_.store(game_->intensity());
            }
            if (c) bus.publish(*c);
        } else {
            // Nothing ready; sleep briefly. ~5 ms keeps latency low, CPU near 0.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

} // namespace hypeclip
