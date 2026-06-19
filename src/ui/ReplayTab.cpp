#include "ui/ReplayTab.hpp"
#include "ui/UiUtil.hpp"
#include "core/Config.hpp"
#include "core/PipelineController.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>

namespace hypeclip {

ReplayTab::ReplayTab(QWidget* parent) : QWidget(parent) {
    loading_ = true;
    const Settings s = Config::instance().get();
    const auto& r = s.replay;
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12,12,12,12);

    // Clip length mode (Priority #4).
    auto* modeG = new QGroupBox("Clip length");
    auto* modeF = new QFormLayout(modeG);
    mode_ = new QComboBox();
    mode_->addItem("OBS Native Replay Buffer (use my OBS settings)");
    mode_->addItem("Custom clip engine (plugin decides length)");
    mode_->setCurrentIndex(r.mode == ReplayMode::CustomClip ? 1 : 0);
    modeF->addRow("Mode", mode_);
    customBox_ = new QWidget();
    auto* cf = new QFormLayout(customBox_); cf->setContentsMargins(0,0,0,0);
    pre_ = new QSpinBox();  pre_->setRange(3,120); pre_->setValue(r.preSeconds);  pre_->setSuffix(" s before");
    post_= new QSpinBox(); post_->setRange(2,120); post_->setValue(r.postSeconds); post_->setSuffix(" s after");
    cf->addRow("Pre-roll", pre_); cf->addRow("Post-roll", post_);
    modeF->addRow(customBox_);
    root->addWidget(modeG);

    // Replay scene + transitions (Priority #3).
    auto* scG = new QGroupBox("Replay scene & transitions");
    auto* scF = new QFormLayout(scG);
    scene_ = new QComboBox();
    scene_->addItem("(HypeClip minimal scene — clean INSTANT REPLAY)", QString());
    for (const QString& n : ui::sceneNames()) scene_->addItem(n, n);
    if (!r.replaySceneName.empty()) scene_->setCurrentText(QString::fromStdString(r.replaySceneName));
    scF->addRow("Replay scene", scene_);

    const QStringList trs = ui::transitionNames();
    enterTr_ = new QComboBox(); enterTr_->addItems(trs);
    enterTr_->setCurrentText(QString::fromStdString(r.enterTransition));
    returnTr_ = new QComboBox(); returnTr_->addItems(trs);
    returnTr_->setCurrentText(QString::fromStdString(r.returnTransition));
    enterMs_ = new QSpinBox(); enterMs_->setRange(0,3000); enterMs_->setSingleStep(50); enterMs_->setSuffix(" ms"); enterMs_->setValue(r.enterTransitionMs);
    returnMs_= new QSpinBox(); returnMs_->setRange(0,3000); returnMs_->setSingleStep(50); returnMs_->setSuffix(" ms"); returnMs_->setValue(r.returnTransitionMs);
    duration_= new QSpinBox(); duration_->setRange(1000,30000); duration_->setSingleStep(500); duration_->setSuffix(" ms"); duration_->setValue(r.replayDurationMs);
    scF->addRow("Enter transition", enterTr_);
    scF->addRow("Enter duration", enterMs_);
    scF->addRow("Replay hold", duration_);
    scF->addRow("Return transition", returnTr_);
    scF->addRow("Return duration", returnMs_);
    showLabel_ = new QCheckBox("Show centered \"INSTANT REPLAY\" label");
    showLabel_->setChecked(r.showLabel);
    scF->addRow(showLabel_);
    root->addWidget(scG);

    auto* note = new QLabel("No captions, commentary or hype text are ever drawn — only the single label above.");
    note->setWordWrap(true); note->setStyleSheet("color:#9aa0aa;");
    root->addWidget(note);
    root->addStretch();

    onModeChanged();
    loading_ = false;

    connect(mode_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]{ onModeChanged(); pushToConfig(); });
    for (QComboBox* c : {scene_, enterTr_, returnTr_})
        connect(c, qOverload<int>(&QComboBox::currentIndexChanged), this, &ReplayTab::pushToConfig);
    for (QSpinBox* sp : {pre_, post_, enterMs_, returnMs_, duration_})
        connect(sp, qOverload<int>(&QSpinBox::valueChanged), this, &ReplayTab::pushToConfig);
    connect(showLabel_, &QCheckBox::toggled, this, &ReplayTab::pushToConfig);
}

void ReplayTab::onModeChanged() { customBox_->setVisible(mode_->currentIndex() == 1); }

void ReplayTab::pushToConfig() {
    if (loading_) return;
    Settings s = Config::instance().get();
    auto& r = s.replay;
    r.mode = mode_->currentIndex()==1 ? ReplayMode::CustomClip : ReplayMode::NativeBuffer;
    r.preSeconds = pre_->value(); r.postSeconds = post_->value();
    r.replaySceneName = scene_->currentData().toString().toStdString();
    r.enterTransition = enterTr_->currentText().toStdString();
    r.returnTransition = returnTr_->currentText().toStdString();
    r.enterTransitionMs = enterMs_->value(); r.returnTransitionMs = returnMs_->value();
    r.replayDurationMs = duration_->value();
    r.showLabel = showLabel_->isChecked();
    Config::instance().set(s);
    PipelineController::instance().reconfigure();
}

} // namespace hypeclip
