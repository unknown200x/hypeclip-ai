#pragma once
// Top-level dock widget: a dark, native-styled QTabWidget hosting the
// Dashboard / Highlights / Settings panes (Replay Center, Audio, Vision and
// Performance panes are added as the corresponding modules graduate from stub).
//
// registerDock() builds the widget and registers it with OBS via
// obs_frontend_add_dock_by_id so its visibility/geometry persist across runs.
#include <QFrame>

namespace hypeclip {

class HypeClipDock : public QFrame {
    Q_OBJECT
public:
    explicit HypeClipDock(QWidget* parent = nullptr);

    // Called once from obs_module_load.
    static void registerDock();
};

} // namespace hypeclip
