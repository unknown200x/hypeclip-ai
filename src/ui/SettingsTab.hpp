#pragma once
#include <QWidget>
class QSlider; class QSpinBox; class QCheckBox; class QComboBox; class QLineEdit; class QLabel;

namespace hypeclip {
class SettingsTab : public QWidget {
    Q_OBJECT
public:
    explicit SettingsTab(QWidget* parent = nullptr);
private slots:
    void pushToConfig();
    void resetDefaults();
private:
    QComboBox* mode_ = nullptr;
    QSlider*   threshold_ = nullptr;
    QLabel*    thrVal_ = nullptr;
    QSpinBox*  minGap_ = nullptr;
    QSpinBox*  retention_ = nullptr;
    QCheckBox* fpLearning_ = nullptr;
    QLineEdit* clipDir_ = nullptr;
    bool loading_ = false;
};
} // namespace hypeclip
