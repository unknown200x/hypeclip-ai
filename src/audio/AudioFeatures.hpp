#pragma once
// Lightweight per-frame audio feature extraction.
//
// Everything here is O(N) over a short window (≈20 ms) of mono float samples
// and uses only cheap time-domain + a small Goertzel band bank, so the whole
// analysis thread stays well under the <2% CPU budget. No FFT library needed.
#include <vector>
#include <cstddef>

namespace hypeclip {

struct FrameFeatures {
    float rms        = 0.0f;  // 0..1 loudness
    float peak       = 0.0f;  // 0..1 instantaneous peak
    float crest      = 0.0f;  // peak/rms — transients (gunfire/explosions) are spiky
    float zcr        = 0.0f;  // zero-crossing rate — proxy for brightness/pitch
    float flux       = 0.0f;  // spectral flux vs previous frame — onset strength
    float lowEnergy  = 0.0f;  // 60–250 Hz band energy (explosions, bass thumps)
    float voiceEnergy= 0.0f;  // 300–3400 Hz band energy (human voice)
    float highEnergy = 0.0f;  // 4–8 kHz band energy (screams, sibilance, gunshots)
};

// Stateful so it can compute deltas (flux) across frames.
class FeatureExtractor {
public:
    explicit FeatureExtractor(int sampleRate = 48000);
    void  setSampleRate(int sr);
    FrameFeatures analyze(const float* mono, size_t n);

private:
    float goertzel(const float* x, size_t n, float freqHz) const;
    int   sampleRate_;
    float prevLow_ = 0, prevVoice_ = 0, prevHigh_ = 0;  // for spectral flux
};

// Utility: downmix interleaved planar/float channels to mono in-place buffer.
void downmixToMono(const float* const* planes, int channels, size_t frames,
                   std::vector<float>& out);

} // namespace hypeclip
