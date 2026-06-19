// Dependency-free unit tests for HypeClip AI core logic (no OBS/Qt).
#include "audio/AudioFeatures.hpp"
#include "audio/VoiceExcitementDetector.hpp"
#include "audio/GameAudioDetector.hpp"
#include "scoring/MomentumTracker.hpp"
#include "scoring/ConfidenceEngine.hpp"
#include "scoring/SessionLearner.hpp"
#include "rules/RuleEngine.hpp"
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
    for (int i = 0; i < n; ++i) v[i] = amp * std::sin(2.0f*3.14159265f*hz*i/sr);
    return v;
}

static void test_features() {
    std::printf("[features]\n");
    FeatureExtractor fx(48000);
    auto loud = tone(500, 0.8f, 960), quiet = tone(500, 0.02f, 960);
    auto a = fx.analyze(loud.data(), loud.size());
    auto b = fx.analyze(quiet.data(), quiet.size());
    CHECK(a.rms > b.rms, "louder -> higher RMS");
    CHECK(a.voiceEnergy > b.voiceEnergy, "voice band tracks amplitude");
}

static void test_voice_scores() {
    std::printf("[voice]\n");
    VoiceExcitementDetector det; TimePoint t = Clock::now();
    for (int i=0;i<200;++i){ FrameFeatures f; f.rms=0.05f; f.voiceEnergy=0.05f; f.peak=0.1f; det.analyze(f,t); t+=Millis(20);}
    MicScores last{};
    for (int i=0;i<12;++i){ FrameFeatures f; f.rms=0.6f; f.voiceEnergy=0.5f; f.highEnergy=0.5f; f.peak=0.95f; last=det.analyze(f,t); t+=Millis(20);}
    CHECK(last.excitement > 50, "loud spike -> high excitement");
    CHECK(last.peak > 80, "peak score tracks sample peak");
    CHECK(last.scream > 50, "bright loud voiced -> scream score");
}

static void test_momentum() {
    std::printf("[momentum]\n");
    MomentumTracker m; TimePoint t = Clock::now();
    Contribution c; c.source=TriggerSource::GameAudio; c.type=HighlightType::Kill; c.energy=0.6f;
    for (int i=0;i<5;++i){ c.when=t; m.add(c); t+=Millis(300);}
    CHECK(m.recentEventCount() >= 3, "events counted");
    auto esc = m.escalateType(HighlightType::Kill);
    CHECK(esc==HighlightType::MultiKill || esc==HighlightType::Ace, "kills escalate");
}

static void test_confidence() {
    std::printf("[confidence]\n");
    ConfidenceEngine ce; Weights w;
    LayerScores single; single.audio=50;
    LayerScores both;   both.audio=35; both.voice=35;
    CHECK(ce.fuse(single,w) < 50, "single modality damped");
    CHECK(ce.fuse(both,w) > ce.fuse(single,w), "cross-modal scores higher");
}

static void test_rules() {
    std::printf("[rules]\n");
    RuleEngine re;
    Rule r; r.name="Hype"; r.join=RuleJoin::All; r.action=RuleAction::CreateClip; r.cooldownMs=1000;
    r.conditions = { {Metric::MicExcitement, Comparator::Greater, 80},
                     {Metric::GameSpike,     Comparator::Greater, 60} };
    re.setRules({r});
    TimePoint t = Clock::now();
    MetricSnapshot lo; lo.micExcitement=85; lo.gameSpike=40;   // only one met
    CHECK(!re.evaluate(lo,t).has_value(), "AND not satisfied -> no fire");
    MetricSnapshot hi; hi.micExcitement=85; hi.gameSpike=70;   // both met
    auto hit = re.evaluate(hi,t);
    CHECK(hit.has_value(), "AND satisfied -> fire");
    CHECK(!re.evaluate(hi, t+Millis(100)).has_value(), "cooldown blocks refire");
    CHECK(re.evaluate(hi, t+Millis(1100)).has_value(), "fires again after cooldown");
    CHECK(re.bestProgress(hi) > re.bestProgress(lo), "progress reflects closeness");
}

static void test_learner() {
    std::printf("[false-positive]\n");
    SessionLearner sl; TimePoint t = Clock::now();
    MetricSnapshot calm; calm.micExcitement=15;
    for (int i=0;i<50;++i){ sl.observe(calm, t); t+=Millis(20);}
    MetricSnapshot pop; pop.micPeak=85; pop.reactionScore=10; pop.micExcitement=40;
    CHECK(sl.shouldReject(pop, /*sustainedMs*/0, /*modalities*/1), "cough/pop rejected");
    MetricSnapshot real; real.micExcitement=90; real.reactionScore=70; real.gameIntensity=60;
    CHECK(!sl.shouldReject(real, 600, 2), "real corroborated moment accepted");
}

static void test_metadata() {
    std::printf("[metadata]\n");
    HighlightEvent e; e.type=HighlightType::Ace; e.confidence=96;
    Contribution mic; mic.source=TriggerSource::MicVoice; e.contributors.push_back(mic);
    auto title = AutoTitler::make(e, "Valorant");
    auto tags  = AutoTagger::make(e, "Valorant");
    CHECK(title.find("ACE")!=std::string::npos, "ace title");
    bool g=false; for (auto& s: tags) if (s=="Valorant") g=true;
    CHECK(g, "tags include game");
}

int main() {
    std::printf("HypeClip AI unit tests (v2)\n");
    test_features(); test_voice_scores(); test_momentum(); test_confidence();
    test_rules(); test_learner(); test_metadata();
    std::printf("\n%d/%d checks passed\n", g_total-g_fail, g_total);
    return g_fail==0 ? 0 : 1;
}
