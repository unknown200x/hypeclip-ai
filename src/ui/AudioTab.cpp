#include "ui/AudioTab.hpp"
#include "ui/UiUtil.hpp"
#include "core/Config.hpp"
#include "core/MetricsHub.hpp"
#include "core/PipelineController.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QShowEvent>
#include <algorithm>

namespace hypeclip {

static void fillSourceList(QListWidget* lw, const std::vector<std::string>& selected) {
    lw->clear();
    for (const QString& n : ui::audioSources()) {
        auto* it = new QListWidgetItem(n, lw);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        bool on = std::find(selected.begin(), selected.end(), n.toStdString()) != selected.end();
        it->setCheckState(on ? Qt::Checked : Qt::Unchecked);
    }
    if (lw->count() == 0) {
        auto* hint = new QListWidgetItem("(no audio sources yet - add a Mic/Desktop Audio in OBS, then click Refresh)", lw);
        hint->setFlags(Qt::NoItemFlags);
    }
}

QSlider* AudioTab::addSlider(QFormLayout* form, const QString& key, const QString& label, int val) {
    auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0,0,0,0);
    auto* sl = new QSlider(Qt::Horizontal); sl->setRange(0,100); sl->setValue(val);
    auto* num = new QLabel(QString::number(val)); num->setFixedWidth(28);
    connect(sl, &QSlider::valueChanged, num, [num](int v){ num->setText(QString::number(v)); });
    connect(sl, &QSlider::valueChanged, this, &AudioTab::pushToConfig);
    h->addWidget(sl); h->addWidget(num);
    form->addRow(label, row);
    sliders_[key] = sl;
    return sl;
}

void AudioTab::refreshSources() {
    loading_ = true;
    const Settings s = Config::instance().get();
    fillSourceList(micList_, s.micSources);
    fillSourceList(gameList_, s.gameSources);
    loading_ = false;
}

void AudioTab::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    refreshSources();   // OBS audio devices exist by the time you open this tab
}

AudioTab::AudioTab(QWidget* parent) : QWidget(parent) {
    loading_ = true;
    const Settings s = Config::instance().get();
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12,12,12,12);

    auto* refresh = new QPushButton("Refresh device list");
    connect(refresh, &QPushButton::clicked, this, [this]{ refreshSources(); });
    root->addWidget(refresh);

    auto* micG = new QGroupBox("Microphone(s) - tick the inputs to monitor");
    auto* micV = new QVBoxLayout(micG);
    micList_ = new QListWidget(); micList_->setMaximumHeight(110);
    micV->addWidget(micList_);
    auto* micForm = new QFormLayout();
    const auto& mt = s.micThresholds;
    addSlider(micForm, "voice_act", "Voice activation", (int)mt.voiceActivation);
    addSlider(micForm, "excitement", "Excitement", (int)mt.excitement);
    addSlider(micForm, "peak", "Peak", (int)mt.peak);
    addSlider(micForm, "scream", "Scream", (int)mt.scream);
    addSlider(micForm, "laughter", "Laughter", (int)mt.laughter);
    addSlider(micForm, "reaction", "Reaction", (int)mt.reaction);
    micV->addLayout(micForm);
    micMeter_ = new QLabel("Mic level -"); micMeter_->setStyleSheet("color:#39d98a;");
    micV->addWidget(micMeter_);
    root->addWidget(micG);

    auto* gG = new QGroupBox("Game / Desktop audio source(s)");
    auto* gV = new QVBoxLayout(gG);
    gameList_ = new QListWidget(); gameList_->setMaximumHeight(110);
    gV->addWidget(gameList_);
    auto* gForm = new QFormLayout();
    const auto& gt = s.gameTuning;
    addSlider(gForm, "sensitivity", "Audio sensitivity", (int)gt.sensitivity);
    addSlider(gForm, "event_sens", "Event sensitivity", (int)gt.eventSensitivity);
    addSlider(gForm, "spike", "Spike detection", (int)gt.spikeDetection);
    addSlider(gForm, "clustering", "Sound clustering", (int)gt.clustering);
    cooldown_ = new QSpinBox(); cooldown_->setRange(100, 5000); cooldown_->setSingleStep(100);
    cooldown_->setValue(gt.cooldownMs); cooldown_->setSuffix(" ms");
    connect(cooldown_, qOverload<int>(&QSpinBox::valueChanged), this, &AudioTab::pushToConfig);
    gForm->addRow("Cooldown window", cooldown_);
    gV->addLayout(gForm);
    gameMeter_ = new QLabel("Game intensity -"); gameMeter_->setStyleSheet("color:#39d98a;");
    gV->addWidget(gameMeter_);
    root->addWidget(gG);
    root->addStretch();

    connect(micList_, &QListWidget::itemChanged, this, &AudioTab::pushToConfig);
    connect(gameList_, &QListWidget::itemChanged, this, &AudioTab::pushToConfig);

    refreshSources();
    loading_ = false;

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &AudioTab::poll);
    timer_->start(150);
}

void AudioTab::pushToConfig() {
    if (loading_) return;
    Settings s = Config::instance().get();
    s.micSources.clear();
    for (int i=0;i<micList_->count();++i) if (micList_->item(i)->checkState()==Qt::Checked)
        s.micSources.push_back(micList_->item(i)->text().toStdString());
    s.gameSources.clear();
    for (int i=0;i<gameList_->count();++i) if (gameList_->item(i)->checkState()==Qt::Checked)
        s.gameSources.push_back(gameList_->item(i)->text().toStdString());
    auto& mt = s.micThresholds;
    mt.voiceActivation = sliders_["voice_act"]->value(); mt.excitement = sliders_["excitement"]->value();
    mt.peak = sliders_["peak"]->value(); mt.scream = sliders_["scream"]->value();
    mt.laughter = sliders_["laughter"]->value(); mt.reaction = sliders_["reaction"]->value();
    auto& gt = s.gameTuning;
    gt.sensitivity = sliders_["sensitivity"]->value(); gt.eventSensitivity = sliders_["event_sens"]->value();
    gt.spikeDetection = sliders_["spike"]->value(); gt.clustering = sliders_["clustering"]->value();
    gt.cooldownMs = cooldown_->value();
    Config::instance().set(s);
    PipelineController::instance().reconfigure();
}

void AudioTab::poll() {
    const MetricSnapshot m = MetricsHub::instance().get();
    micMeter_->setText(QString("Mic level %1   .   excitement %2   .   scream %3")
        .arg((int)m.micLevel).arg((int)m.micExcitement).arg((int)m.screamScore));
    gameMeter_->setText(QString("Game intensity %1   .   spike %2")
        .arg((int)m.gameIntensity).arg((int)m.gameSpike));
}

} // namespace hypeclip
