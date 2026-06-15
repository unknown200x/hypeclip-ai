#pragma once
// Settings: Beginner/Creator modes. Beginner hides advanced controls and uses
// defaults; Creator exposes threshold, pre/post seconds, sources, replay style,
// vision, commentary and retention. Writes through to Config and hot-applies
// via PipelineController::reconfigure().
#include <QWidget>

class QComboBox;
class QSlider;
class QSpinBox;
class QCheckBox;
class QGroupBox;

namespace hypeclip {

class SettingsTab : public QWidget {
    Q_OBJECT
public:
    explicit SettingsTab(QWidget* parent = nullptr);

private slots:
    void onModeChanged(int idx);
    void apply();

private:
    void populateAudioSources();
    void loadFromConfig();

    QComboBox* mode_ = nullptr;
    QSlider*   threshold_ = nullptr;
    QSpinBox*  pre_ = nullptr;
    QSpinBox*  post_ = nullptr;
    QComboBox* mic_ = nullptr;
    QComboBox* game_ = nullptr;
    QComboBox* style_ = nullptr;
    QCheckBox* vision_ = nullptr;
    QCheckBox* commentary_ = nullptr;
    QSpinBox*  retention_ = nullptr;
    QGroupBox* advanced_ = nullptr;
};

} // namespace hypeclip
