#include "ui/DashboardTab.hpp"
#include "ui/HypeMeter.hpp"
#include "core/PipelineController.hpp"
#include "core/Config.hpp"

#include <obs-module.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <algorithm>

namespace hypeclip {

DashboardTab::DashboardTab(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    status_ = new QLabel(obs_module_text("Dashboard.Status.Armed"));
    status_->setStyleSheet("font-weight:600; color:#39d98a;");
    root->addWidget(status_);

    auto addMeter = [&](const char* labelKey, HypeMeter*& m) {
        auto* row = new QVBoxLayout();
        row->addWidget(new QLabel(obs_module_text(labelKey)));
        m = new HypeMeter();
        row->addWidget(m);
        root->addLayout(row);
    };
    addMeter("Dashboard.HypeMeter", hype_);
    addMeter("Dashboard.ClipWorthiness", clipWorthiness_);

    clipsToday_ = new QLabel(QString(obs_module_text("Dashboard.ClipsToday")) + ": 0");
    root->addWidget(clipsToday_);

    rbWarning_ = new QLabel(obs_module_text("Dashboard.ReplayBufferOff"));
    rbWarning_->setWordWrap(true);
    rbWarning_->setStyleSheet("color:#e0a030;");
    rbWarning_->setVisible(false);
    root->addWidget(rbWarning_);

    enableRb_ = new QPushButton(obs_module_text("Dashboard.EnableReplayBuffer"));
    enableRb_->setVisible(false);
    connect(enableRb_, &QPushButton::clicked, this, &DashboardTab::enableReplayBuffer);
    root->addWidget(enableRb_);

    root->addStretch();

    const float thr = Config::instance().get().threshold / 100.0f;
    hype_->setThreshold(thr);
    clipWorthiness_->setThreshold(thr);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &DashboardTab::poll);
    timer_->start(120);   // ~8 Hz UI refresh; negligible CPU
}

void DashboardTab::enableReplayBuffer() {
    PipelineController::instance().replay().ensureActive();
}

void DashboardTab::poll() {
    auto& pc = PipelineController::instance();
    const float mic = pc.micLevel();
    const float game = pc.gameLevel();
    hype_->setValue(std::max(mic, game));
    clipWorthiness_->setValue(pc.confidence() / 100.0f);

    const bool rbActive = pc.replay().isActive();
    rbWarning_->setVisible(!rbActive);
    enableRb_->setVisible(!rbActive);
    status_->setText(rbActive ? obs_module_text("Dashboard.Status.Armed")
                              : obs_module_text("Dashboard.Status.Idle"));

    clipsToday_->setText(QString(obs_module_text("Dashboard.ClipsToday")) + ": " +
                         QString::number((int)pc.clips().size()));
}

} // namespace hypeclip
