#include "ui/SettingsTab.hpp"
#include "core/Config.hpp"
#include "core/PipelineController.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

namespace hypeclip {

SettingsTab::SettingsTab(QWidget* parent) : QWidget(parent) {
    loading_ = true;
    const Settings s = Config::instance().get();
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12,12,12,12);

    auto* g = new QGroupBox("General");
    auto* form = new QFormLayout(g);

    mode_ = new QComboBox(); mode_->addItem("Beginner (works instantly)"); mode_->addItem("Creator (full control)");
    mode_->setCurrentIndex(s.mode == Mode::Creator ? 1 : 0);
    form->addRow("Mode", mode_);

    auto* thrRow = new QWidget(); auto* th = new QHBoxLayout(thrRow); th->setContentsMargins(0,0,0,0);
    threshold_ = new QSlider(Qt::Horizontal); threshold_->setRange(40,100); threshold_->setValue((int)s.threshold);
    thrVal_ = new QLabel(QString::number((int)s.threshold)); thrVal_->setFixedWidth(28);
    connect(threshold_, &QSlider::valueChanged, thrVal_, [this](int v){ thrVal_->setText(QString::number(v)); });
    th->addWidget(threshold_); th->addWidget(thrVal_);
    form->addRow("AI clip threshold", thrRow);

    minGap_ = new QSpinBox(); minGap_->setRange(1000,60000); minGap_->setSingleStep(500);
    minGap_->setSuffix(" ms"); minGap_->setValue(s.minClipGapMs);
    form->addRow("Min spacing between clips", minGap_);

    retention_ = new QSpinBox(); retention_->setRange(0,365); retention_->setValue(s.retentionDays);
    retention_->setSpecialValueText("Keep forever");
    form->addRow("Delete clips after (days)", retention_);

    fpLearning_ = new QCheckBox("Learn session noise & block false positives (coughs, keyboard, pings)");
    fpLearning_->setChecked(s.falsePositiveLearning);
    form->addRow("", fpLearning_);

    clipDir_ = new QLineEdit(QString::fromStdString(s.clipDirectory));
    clipDir_->setPlaceholderText("Leave blank to use your OBS recording folder");
    form->addRow("Clip folder", clipDir_);
    root->addWidget(g);

    auto* reset = new QPushButton("Reset to defaults");
    root->addWidget(reset);
    root->addStretch();
    loading_ = false;

    connect(mode_, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsTab::pushToConfig);
    connect(threshold_, &QSlider::valueChanged, this, &SettingsTab::pushToConfig);
    connect(minGap_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsTab::pushToConfig);
    connect(retention_, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsTab::pushToConfig);
    connect(fpLearning_, &QCheckBox::toggled, this, &SettingsTab::pushToConfig);
    connect(clipDir_, &QLineEdit::editingFinished, this, &SettingsTab::pushToConfig);
    connect(reset, &QPushButton::clicked, this, &SettingsTab::resetDefaults);
}

void SettingsTab::pushToConfig() {
    if (loading_) return;
    Settings s = Config::instance().get();
    s.mode = mode_->currentIndex()==1 ? Mode::Creator : Mode::Beginner;
    s.threshold = (float)threshold_->value();
    s.minClipGapMs = minGap_->value();
    s.retentionDays = retention_->value();
    s.falsePositiveLearning = fpLearning_->isChecked();
    s.clipDirectory = clipDir_->text().toStdString();
    Config::instance().set(s);
    PipelineController::instance().reconfigure();
}

void SettingsTab::resetDefaults() {
    Config::instance().applyBeginnerDefaults();
    Config::instance().save();
    PipelineController::instance().reconfigure();
    // Reload visible controls.
    const Settings s = Config::instance().get();
    loading_ = true;
    mode_->setCurrentIndex(0); threshold_->setValue((int)s.threshold);
    minGap_->setValue(s.minClipGapMs); retention_->setValue(s.retentionDays);
    fpLearning_->setChecked(s.falsePositiveLearning); clipDir_->setText("");
    loading_ = false;
}

} // namespace hypeclip
