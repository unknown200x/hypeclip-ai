#pragma once
// Taps ONE OBS audio source (mic or game) on the real-time audio thread, downmixes
// to mono, pushes through a lock-free ring buffer to an analysis thread which
// computes scores, writes the live MetricSnapshot, applies the user's thresholds /
// feature toggles, and emits Contributions. PipelineController creates one instance
// per selected source (Priority #5/#6 multi-device support).
#include "audio/AudioFeatures.hpp"
#include "audio/VoiceExcitementDetector.hpp"
#include "audio/GameAudioDetector.hpp"
#include "util/RingBuffer.hpp"
#include "core/Types.hpp"

#include <atomic>
#include <thread>
#include <string>
#include <memory>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs.h>
#else
struct obs_source;
typedef struct obs_source obs_source_t;
struct audio_data;
#endif

namespace hypeclip {

enum class CaptureRole { Mic, Game };

class AudioCapture {
public:
    explicit AudioCapture(CaptureRole role);
    ~AudioCapture();

    bool attach(const std::string& sourceName);   // empty => auto default for role
    void detach();
    void start();
    void stop();

    float meterLevel() const { return meter_.load(std::memory_order_relaxed); }

#if defined(HYPECLIP_HAVE_OBS)
    static void obsAudioCb(void* param, obs_source_t* source,
                           const struct audio_data* audio, bool muted);
#endif

private:
    void analysisLoop();

    CaptureRole role_;
    obs_source_t* source_ = nullptr;
    std::string   sourceName_;

    RingBuffer<float> ring_{1u << 17};
    std::vector<float> scratch_, downmix_;

    FeatureExtractor extractor_;
    std::unique_ptr<VoiceExcitementDetector> voice_;
    std::unique_ptr<GameAudioDetector> game_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<int>  sampleRate_{48000};
    std::atomic<float> meter_{0.0f};
    TimePoint lastEmit_{};
};

} // namespace hypeclip
