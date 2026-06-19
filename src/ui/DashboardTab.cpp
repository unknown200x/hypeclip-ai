#include "ui/DashboardTab.hpp"
#include "ui/HypeMeter.hpp"
#include "core/Config.hpp"
#include "core/MetricsHub.hpp"
#include "core/PipelineController.hpp"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QTimer>
#include <algorithm>
#include <utility>

namespace hypeclip {

DashboardTab::DashboardTab(QWidget* parent) : QWidget(parent) {
    loading_ = true;
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12,12,12,12); root->setSpacing(10);

    // Master switch.
    master_ = new QCheckBox("Enable HypeClip AI");
    master_->setStyleSheet("font-size:15px;font-weight:700;color:#39d98a;");
    root->addWidget(master_);
    status_ = new QLabel(); status_->setStyleSheet("color:#9aa0aa;");
    root->addWidget(status_);

    // Feature toggles.
    auto* fg = new QGroupBox("Features");
    auto* grid = new QGridLayout(fg);
    const std::pair<QString,QString> defs[] = {
        {"autoClip","Auto Clip Detection"}, {"instantReplay","Instant Replay"},
        {"audio","Audio Detection"}, {"mic","Mic Detection"},
        {"game_audio","Game Audio Detection"}, {"visual","Visual Detection (GPU)"},
        {"ai_scoring","AI Scoring Engine"}, {"eos","End-of-Stream Highlights"},
        {"social","Auto Social Export"},
    };
    int row=0,col=0;
    for (auto& d : defs) {
        auto* cb = new QCheckBox(d.second);
        feats_[d.first] = cb;
        grid->addWidget(cb, row, col);
        if (++col==2){col=0;++row;}
    }
    root->addWidget(fg);

    // Live meters.
    auto* mg = new QGroupBox("Live");
    auto* ml = new QVBoxLayout(mg);
    auto addMeter=[&](const char* name, HypeMeter*& m){
        ml->addWidget(new QLabel(name)); m=new HypeMeter(); ml->addWidget(m);
    };
    addMeter("Hype Meter", hype_);
    addMeter("Clip Worthiness", worth_);
    addMeter("Mic Level", mic_);
    scores_ = new QLabel("—"); scores_->setStyleSheet("color:#9aa0aa;");
    ml->addWidget(scores_);
    root->addWidget(mg);

    rbWarn_ = new QLabel("Replay Buffer is not running — enable it so clips can be saved.");
    rbWarn_->setWordWrap(true); rbWarn_->setStyleSheet("color:#e0a030;"); rbWarn_->setVisible(false);
    root->addWidget(rbWarn_);
    rbBtn_ = new QPushButton("Enable Replay Buffer"); rbBtn_->setVisible(false);
    connect(rbBtn_, &QPushButton::clicked, this, []{ PipelineController::instance().replay().ensureActive(); });
    root->addWidget(rbBtn_);
    root->addStretch();

    // Load current settings into controls.
    const Settings s = Config::instance().get();
    master_->setChecked(s.masterEnabled);
    feats_["autoClip"]->setChecked(s.features.autoClip);
    feats_["instantReplay"]->setChecked(s.features.instantReplay);
    feats_["audio"]->setChecked(s.features.audioDetection);
    feats_["mic"]->setChecked(s.features.micDetection);
    feats_["game_audio"]->setChecked(s.features.gameAudioDetection);
    feats_["visual"]->setChecked(s.features.visualDetection);
    feats_["ai_scoring"]->setChecked(s.features.aiScoring);
    feats_["eos"]->setChecked(s.features.endOfStreamHighlights);
    feats_["social"]->setChecked(s.features.autoSocialExport);
    const float thr = s.threshold/100.0f;
    hype_->setThreshold(thr); worth_->setThreshold(thr);
    loading_ = false;

    connect(master_, &QCheckBox::toggled, this, &DashboardTab::pushToConfig);
    for (auto* cb : feats_) connect(cb, &QCheckBox::toggled, this, &DashboardTab::pushToConfig);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &DashboardTab::poll);
    timer_->start(120);
}

void DashboardTab::pushToConfig() {
    if (loading_) return;
    Settings s = Config::instance().get();
    s.masterEnabled = master_->isChecked();
    s.features.autoClip = feats_["autoClip"]->isChecked();
    s.features.instantReplay = feats_["instantReplay"]->isChecked();
    s.features.audioDetection = feats_["audio"]->isChecked();
    s.features.micDetection = feats_["mic"]->isChecked();
    s.features.gameAudioDetection = feats_["game_audio"]->isChecked();
    s.features.visualDetection = feats_["visual"]->isChecked();
    s.features.aiScoring = feats_["ai_scoring"]->isChecked();
    s.features.endOfStreamHighlights = feats_["eos"]->isChecked();
    s.features.autoSocialExport = feats_["social"]->isChecked();
    Config::instance().set(s);
    PipelineController::instance().reconfigure();
}

void DashboardTab::poll() {
    const MetricSnapshot m = MetricsHub::instance().get();
    hype_->setValue(std::max(m.micExcitement, m.gameIntensity)/100.0f);
    worth_->setValue(m.clipWorthiness/100.0f);
    mic_->setValue(m.micLevel/100.0f);
    scores_->setText(QString("Excitement %1  •  Game %2  •  Confidence %3%  •  Momentum %4")
        .arg((int)m.micExcitement).arg((int)m.gameIntensity).arg((int)m.confidence).arg((int)m.momentum));

    const bool active = PipelineController::instance().isActive();
    const bool rb = PipelineController::instance().replay().isActive();
    status_->setText(!master_->isChecked() ? "Disabled — idle, no resources used"
                     : (active ? "Armed — listening for moments" : "Starting…"));
    rbWarn_->setVisible(master_->isChecked() && !rb);
    rbBtn_->setVisible(master_->isChecked() && !rb);
}

} // namespace hypeclip
