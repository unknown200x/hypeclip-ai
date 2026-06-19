#include "ui/HypeClipDock.hpp"
#include "ui/DashboardTab.hpp"
#include "ui/AudioTab.hpp"
#include "ui/RulesTab.hpp"
#include "ui/ReplayTab.hpp"
#include "ui/HighlightsTab.hpp"
#include "ui/SettingsTab.hpp"
#include "ui/UiUtil.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QVBoxLayout>
#include <QTabWidget>
#include <QScrollArea>
#include <QLabel>

namespace hypeclip {

static QWidget* scrollable(QWidget* inner) {
    auto* sa = new QScrollArea();
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setWidget(inner);
    return sa;
}

HypeClipDock::HypeClipDock(QWidget* parent) : QFrame(parent) {
    setObjectName("HypeClipDock");
    setStyleSheet(ui::darkStyle());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0); root->setSpacing(0);

    auto* header = new QLabel("  HypeClip AI  ·  Never miss a moment.");
    header->setStyleSheet("font-size:13px;font-weight:700;color:#39d98a;padding:7px;background:#16181d;");
    root->addWidget(header);

    auto* tabs = new QTabWidget();
    tabs->addTab(scrollable(new DashboardTab()),  "Dashboard");
    tabs->addTab(scrollable(new AudioTab()),      "Audio");
    tabs->addTab(scrollable(new RulesTab()),      "Rules");
    tabs->addTab(scrollable(new ReplayTab()),     "Replay");
    tabs->addTab(new HighlightsTab(),             "Highlights");
    tabs->addTab(scrollable(new SettingsTab()),   "Settings");
    root->addWidget(tabs);
}

void HypeClipDock::registerDock() {
    auto* dock = new HypeClipDock();
    dock->setWindowTitle("HypeClip AI");
    obs_frontend_add_dock_by_id("hypeclip_ai_dock", "HypeClip AI", dock);
}

} // namespace hypeclip
