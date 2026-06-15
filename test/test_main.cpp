// Dependency-free unit tests for the HypeClip AI core logic (no OBS/Qt).
// Build with -DHYPECLIP_BUILD_TESTS=ON, then `ctest`.
#include "audio/AudioFeatures.hpp"
#include "audio/VoiceExcitementDetector.hpp"
#include "scoring/MomentumTracker.hpp"
#include "scoring/ConfidenceEngine.hpp"
#include "metadata/AutoTitler.hpp"
#include "metadata/AutoTagger.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace hypeclip;

static int g_fail = 0, g_total = 0;
#define CHECK(cond, msg) do { ++g_total; if (!(cond)) { ++g_fail; \
    std::printf("  FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

static std::vector<float> tone(float hz, float amp, int n, int sr = 48000) {
    std::vector<float> v(n);
    for (int i = 0; i < n; ++i) v[i] = amp * std::sin(2.0f * 3.14159265f * hz * i / sr);
    return v;
}

static void test_features() {
    std::printf("[features]\n");
    FeatureExtractor fx(48000);
    auto loud = tone(500.f, 0.8f, 960);
    auto quiet = tone(500.f, 0.02f, 960);
    auto fLoud = fx.analyze(loud.data(), loud.size());
    auto fQuiet = fx.analyze(quiet.data(), quiet.size());
    CHECK(fLoud.rms > fQuiet.rms, "louder tone has higher RMS");
    CHECK(fLoud.voiceEnergy > fQuiet.voiceEnergy, "voice-band energy tracks amplitude");
    CHECK(fLoud.peak <= 1.0f && fLoud.peak >= 0.0f, "peak normalised");
}

static void test_voice_spike() {
    std::printf("[voice]\n");
    VoiceExcitementDetector det;
    TimePoint t = Clock::now();
    // Warm up baseline with calm speech-level frames.
    for (int i = 0; i < 200; ++i) {
        FrameFeatures f; f.rms = 0.05f; f.voiceEnergy = 0.05f; f.peak = 0.1f;
        det.feed(f, t); t += Millis(20);
    }
    bool fired = false;
    // A loud, bright, voiced spike should trigger.
    for (int i = 0; i < 10; ++i) {
        FrameFeatures f; f.rms = 0.5f; f.voiceEnergy = 0.4f; f.highEnergy = 0.4f; f.peak = 0.9f;
        if (det.feed(f, t)) fired = true;
        t += Millis(20);
    }
    CHECK(fired, "loud vocal spike emits a contribution");
    CHECK(det.excitement() > 0.5f, "excitement rises on a spike");
}

static void test_momentum() {
    std::printf("[momentum]\n");
    MomentumTracker m;
    TimePoint t = Clock::now();
    Contribution c; c.source = TriggerSource::GameAudio; c.type = HighlightType::Kill; c.energy = 0.6f;
    float prev = 0.f;
    for (int i = 0; i < 5; ++i) { c.when = t; m.add(c); t += Millis(300);
        CHECK(m.momentum() >= prev - 0.01f, "momentum builds with rapid events"); prev = m.momentum(); }
    CHECK(m.recentEventCount() >= 3, "recent events counted");
    CHECK(m.escalateType(HighlightType::Kill) == HighlightType::MultiKill ||
          m.escalateType(HighlightType::Kill) == HighlightType::Ace, "kills escalate");
}

static void test_confidence() {
    std::printf("[confidence]\n");
    ConfidenceEngine ce; Weights w;
    LayerScores single; single.audio = 50.f;            // one modality only
    LayerScores both;   both.audio = 35.f; both.voice = 35.f; // two agree
    float cSingle = ce.fuse(single, w);
    float cBoth   = ce.fuse(both, w);
    CHECK(cSingle < 50.f, "single modality is damped");
    CHECK(cBoth > cSingle, "cross-modal agreement scores higher");
    LayerScores mom = both; mom.momentum = 1.0f;
    CHECK(ce.fuse(mom, w) > cBoth, "momentum increases confidence");
}

static void test_metadata() {
    std::printf("[metadata]\n");
    HighlightEvent e; e.type = HighlightType::Ace; e.confidence = 96.f;
    Contribution mic; mic.source = TriggerSource::MicVoice; e.contributors.push_back(mic);
    auto title = AutoTitler::make(e, "Valorant");
    auto tags  = AutoTagger::make(e, "Valorant");
    CHECK(title.find("ACE") != std::string::npos, "ace title");
    CHECK(title.find("Valorant") != std::string::npos, "title includes game");
    bool hasGame = false, hasReaction = false;
    for (auto& s : tags) { if (s == "Valorant") hasGame = true; if (s == "reaction") hasReaction = true; }
    CHECK(hasGame, "tags include game");
    CHECK(hasReaction, "mic contributor => reaction tag");
}

int main() {
    std::printf("HypeClip AI unit tests\n");
    test_features();
    test_voice_spike();
    test_momentum();
    test_confidence();
    test_metadata();
    std::printf("\n%d/%d checks passed\n", g_total - g_fail, g_total);
    return g_fail == 0 ? 0 : 1;
}
