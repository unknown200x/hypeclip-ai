#include "ui/SettingsTab.hpp"
#include "core/Config.hpp"
#include "core/PipelineController.hpp"

#include <obs.h>
#include <obs-module.h>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>

namespace hypeclip {

SettingsTab::SettingsTab(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);

    mode_ = new QComboBox();
    mode_->addItem(obs_module_text("Settings.Mode.Beginner"));
    mode_->addItem(obs_module_text("Settings.Mode.Creator"));
    {
        auto* row = new QFormLayout();
        row->addRow(obs_module_text("Settings.Mode"), mode_);
        root->addLayout(row);
    }

    advanced_ = new QGroupBox(obs_module_text("Tab.Settings"));
    auto* form = new QFormLayout(advanced_);

    threshold_ = new QSlider(Qt::Horizontal);
    threshold_->setRange(40, 100);
    form->addRow(obs_module_text("Settings.Threshold"), threshold_);

    pre_ = new QSpinBox();  pre_->setRange(5, 120);
    post_ = new QSpinBox(); post_->setRange(2, 60);
    form->addRow(obs_module_text("Settings.PreSeconds"), pre_);
    form->addRow(obs_module_text("Settings.PostSeconds"), post_);

    mic_  = new QComboBox();
    game_ = new QComboBox();
    form->addRow(obs_module_text("Settings.MicSource"), mic_);
    form->addRow(obs_module_text("Settings.GameAudioSource"), game_);

    style_ = new QComboBox();
    style_->addItems({"Esports", "Hype", "Cinematic", "Streamer", "Retro"});
    form->addRow(obs_module_text("Settings.ReplayStyle"), style_);

    vision_     = new QCheckBox();
    commentary_ = new QCheckBox();
    form->addRow(obs_module_text("Settings.EnableVision"), vision_);
    form->addRow(obs_module_text("Settings.EnableCommentary"), commentary_);

    retention_ = new QSpinBox(); retention_->setRange(0, 365);
    form->addRow(obs_module_text("Settings.RetentionDays"), retention_);

    root->addWidget(advanced_);
    root->addStretch();

    populateAudioSources();
    loadFromConfig();

    connect(mode_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsTab::onModeChanged);
    // Persist on any change.
    auto onChange = [this]{ apply(); };
    connect(threshold_, &QSlider::valueChanged, this, onChange);
    connect(pre_,  qOverload<int>(&QSpinBox::valueChanged), this, onChange);
    connect(post_, qOverload<int>(&QSpinBox::valueChanged), this, onChange);
    connect(mic_,  qOverload<int>(&QComboBox::currentIndexChanged), this, onChange);
    connect(game_, qOverload<int>(&QComboBox::currentIndexChanged), this, onChange);
    connect(style_,qOverload<int>(&QComboBox::currentIndexChanged), this, onChange);
    connect(vision_, &QCheckBox::toggled, this, onChange);
    connect(commentary_, &QCheckBox::toggled, this, onChange);
    connect(retention_, qOverload<int>(&QSpinBox::valueChanged), this, onChange);
}

void SettingsTab::populateAudioSources() {
    mic_->addItem(tr("Auto"), QString());
    game_->addItem(tr("Auto"), QString());
    // Enumerate OBS audio-capable sources.
    obs_enum_sources([](void* d, obs_source_t* src) -> bool {
        auto* self = static_cast<SettingsTab*>(d);
        const uint32_t flags = obs_source_get_output_flags(src);
        if (flags & OBS_SOURCE_AUDIO) {
            const char* n = obs_source_get_name(src);
            self->mic_->addItem(n, QString(n));
            self->game_->addItem(n, QString(n));
        }
        return true;
    }, this);
}

void SettingsTab::loadFromConfig() {
    const Settings s = Config::instance().get();
    mode_->setCurrentIndex(s.mode == Mode::Creator ? 1 : 0);
    threshold_->setValue((int)s.threshold);
    pre_->setValue(s.preSeconds);
    post_->setValue(s.postSeconds);
    style_->setCurrentIndex((int)s.replayStyle);
    vision_->setChecked(s.enableVision);
    commentary_->setChecked(s.enableCommentary);
    retention_->setValue(s.retentionDays);
    if (!s.micSourceName.empty())  mic_->setCurrentText(QString::fromStdString(s.micSourceName));
    if (!s.gameAudioSourceName.empty()) game_->setCurrentText(QString::fromStdString(s.gameAudioSourceName));
    onModeChanged(mode_->currentIndex());
}

void SettingsTab::onModeChanged(int idx) {
    const bool creator = (idx == 1);
    advanced_->setVisible(creator);
    if (!creator) {
        // Beginner: reset to safe defaults.
        Config::instance().applyBeginnerDefaults();
    }
    apply();
}

void SettingsTab::apply() {
    Settings s = Config::instance().get();
    s.mode = mode_->currentIndex() == 1 ? Mode::Creator : Mode::Beginner;
    if (s.mode == Mode::Creator) {
        s.threshold   = (float)threshold_->value();
        s.preSeconds  = pre_->value();
        s.postSeconds = post_->value();
        s.replayStyle = (ReplayStyle)style_->currentIndex();
        s.enableVision = vision_->isChecked();
        s.enableCommentary = commentary_->isChecked();
        s.retentionDays = retention_->value();
        s.micSourceName  = mic_->currentData().toString().toStdString();
        s.gameAudioSourceName = game_->currentData().toString().toStdString();
    }
    Config::instance().set(s);
    PipelineController::instance().reconfigure();
}

} // namespace hypeclip
