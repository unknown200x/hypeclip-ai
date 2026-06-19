#pragma once
#include <QWidget>
class QComboBox; class QSpinBox; class QCheckBox; class QWidget;

namespace hypeclip {
class ReplayTab : public QWidget {
    Q_OBJECT
public:
    explicit ReplayTab(QWidget* parent = nullptr);
private slots:
    void pushToConfig();
    void onModeChanged();
private:
    QComboBox* mode_ = nullptr;
    QSpinBox*  pre_ = nullptr;
    QSpinBox*  post_ = nullptr;
    QWidget*   customBox_ = nullptr;
    QComboBox* scene_ = nullptr;
    QComboBox* enterTr_ = nullptr;
    QSpinBox*  enterMs_ = nullptr;
    QSpinBox*  duration_ = nullptr;
    QComboBox* returnTr_ = nullptr;
    QSpinBox*  returnMs_ = nullptr;
    QCheckBox* showLabel_ = nullptr;
    bool loading_ = false;
};
} // namespace hypeclip
