#include "audio/AudioFeatures.hpp"
#include <cmath>
#include <algorithm>

namespace hypeclip {

FeatureExtractor::FeatureExtractor(int sampleRate) : sampleRate_(sampleRate) {}
void FeatureExtractor::setSampleRate(int sr) { sampleRate_ = sr > 0 ? sr : 48000; }

// Single-frequency Goertzel: magnitude of one DFT bin in O(N), no FFT.
// We average a few probe tones per band to approximate band energy cheaply.
float FeatureExtractor::goertzel(const float* x, size_t n, float freqHz) const {
    const float w = 2.0f * 3.14159265358979f * freqHz / (float)sampleRate_;
    const float coeff = 2.0f * std::cos(w);
    float s0 = 0, s1 = 0, s2 = 0;
    for (size_t i = 0; i < n; ++i) {
        s0 = x[i] + coeff * s1 - s2;
        s2 = s1; s1 = s0;
    }
    float power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return power > 0 ? std::sqrt(power) / (float)n : 0.0f;
}

FrameFeatures FeatureExtractor::analyze(const float* x, size_t n) {
    FrameFeatures f;
    if (!x || n == 0) return f;

    double sumSq = 0; float peak = 0; size_t zc = 0;
    for (size_t i = 0; i < n; ++i) {
        const float s = x[i];
        sumSq += (double)s * s;
        peak = std::max(peak, std::fabs(s));
        if (i && ((x[i] >= 0) != (x[i - 1] >= 0))) ++zc;
    }
    f.rms  = (float)std::sqrt(sumSq / (double)n);
    f.peak = peak;
    f.crest = f.rms > 1e-6f ? f.peak / f.rms : 0.0f;
    f.zcr  = (float)zc / (float)n;

    // Cheap 3-band probe bank (averaged Goertzel tones per band).
    auto band = [&](std::initializer_list<float> freqs) {
        float acc = 0; for (float fr : freqs) acc += goertzel(x, n, fr);
        return acc / (float)freqs.size();
    };
    f.lowEnergy   = band({80.f, 150.f, 220.f});
    f.voiceEnergy = band({350.f, 700.f, 1200.f, 2500.f});
    f.highEnergy  = band({4500.f, 6000.f, 7500.f});

    // Spectral flux = positive change in band energies (onset detector).
    auto pos = [](float a, float b){ return std::max(0.0f, a - b); };
    f.flux = pos(f.lowEnergy, prevLow_) + pos(f.voiceEnergy, prevVoice_) +
             pos(f.highEnergy, prevHigh_);
    prevLow_ = f.lowEnergy; prevVoice_ = f.voiceEnergy; prevHigh_ = f.highEnergy;
    return f;
}

void downmixToMono(const float* const* planes, int channels, size_t frames,
                   std::vector<float>& out) {
    out.resize(frames);
    if (channels <= 0 || !planes || !planes[0]) { std::fill(out.begin(), out.end(), 0.f); return; }
    const float inv = 1.0f / (float)channels;
    for (size_t i = 0; i < frames; ++i) {
        float acc = 0;
        for (int c = 0; c < channels; ++c)
            if (planes[c]) acc += planes[c][i];
        out[i] = acc * inv;
    }
}

} // namespace hypeclip
