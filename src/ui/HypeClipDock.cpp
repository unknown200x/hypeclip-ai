#include "ui/HypeClipDock.hpp"
#include "ui/DashboardTab.hpp"
#include "ui/HighlightsTab.hpp"
#include "ui/SettingsTab.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>

namespace hypeclip {

HypeClipDock::HypeClipDock(QWidget* parent) : QFrame(parent) {
    setObjectName("HypeClipDock");
    // Native-ish dark styling; OBS already applies a dark theme, we just refine.
    setStyleSheet(
        "#HypeClipDock { background:#1b1d23; }"
        "QTabBar::tab { padding:6px 12px; }"
        "QLabel { color:#d6d8de; }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* header = new QLabel("  HypeClip AI  ·  " +
                              QString(obs_module_text("HypeClipAI.Tagline")));
    header->setStyleSheet("font-size:13px; font-weight:700; color:#39d98a; padding:6px;");
    root->addWidget(header);

    auto* tabs = new QTabWidget();
    tabs->addTab(new DashboardTab(),  obs_module_text("Tab.Dashboard"));
    tabs->addTab(new HighlightsTab(), obs_module_text("Tab.Highlights"));
    tabs->addTab(new SettingsTab(),   obs_module_text("Tab.Settings"));
    root->addWidget(tabs);
}

void HypeClipDock::registerDock() {
    auto* dock = new HypeClipDock();
    dock->setWindowTitle(obs_module_text("HypeClipAI.Dock"));
    // OBS 30+ takes ownership of the QWidget and persists the dock by id.
    obs_frontend_add_dock_by_id("hypeclip_ai_dock",
                                obs_module_text("HypeClipAI.Dock"), dock);
}

} // namespace hypeclip
