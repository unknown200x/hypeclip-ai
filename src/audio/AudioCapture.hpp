#pragma once
// Taps an OBS audio source (mic or game) via obs_source_add_audio_capture_callback,
// downmixes to mono on the (real-time) audio thread, and forwards samples through
// a lock-free ring buffer to a dedicated analysis thread.
//
// Two instances are created by the PipelineController: one for the mic source and
// one for the game-audio source. The OBS audio callback does NOTHING heavy — it
// only downmixes and pushes — to protect stream/render timing.
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
#include <obs.h>            // real obs_source_t / audio_data when building the plugin
#else
struct obs_source;          // fwd decls for host unit-test builds (no OBS)
typedef struct obs_source obs_source_t;
struct audio_data;
#endif

namespace hypeclip {

enum class CaptureRole { Mic, Game };

class AudioCapture {
public:
    explicit AudioCapture(CaptureRole role);
    ~AudioCapture();

    // Attach to a named OBS source. Empty name => auto-pick a sensible default.
    bool attach(const std::string& sourceName);
    void detach();

    void start();   // spawns analysis thread
    void stop();

    float meterLevel() const;  // 0..1 for the UI (excitement or game intensity)

#if defined(HYPECLIP_HAVE_OBS)
    // OBS C callback trampoline (public so it can be registered).
    static void obsAudioCb(void* param, obs_source_t* source,
                           const struct audio_data* audio, bool muted);
#endif

private:
    void analysisLoop();

    CaptureRole role_;
    obs_source_t* source_ = nullptr;
    std::string   sourceName_;

    RingBuffer<float> ring_{1u << 17};
    std::vector<float> scratch_;     // analysis-thread scratch
    std::vector<float> downmix_;     // audio-thread scratch

    FeatureExtractor extractor_;
    std::unique_ptr<VoiceExcitementDetector> voice_;
    std::unique_ptr<GameAudioDetector> game_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<int>  sampleRate_{48000};
    std::atomic<float> meter_{0.0f};
};

} // namespace hypeclip
