#pragma once
#include <QWidget>
#include <QMap>
#include <QString>
class QLabel; class QPushButton; class QTimer; class QCheckBox;

namespace hypeclip {
class HypeMeter;

class DashboardTab : public QWidget {
    Q_OBJECT
public:
    explicit DashboardTab(QWidget* parent = nullptr);
private slots:
    void poll();
    void pushToConfig();
private:
    QCheckBox* master_ = nullptr;
    QMap<QString, QCheckBox*> feats_;
    HypeMeter* hype_ = nullptr;
    HypeMeter* worth_ = nullptr;
    HypeMeter* mic_ = nullptr;
    QLabel* scores_ = nullptr;
    QLabel* status_ = nullptr;
    QLabel* rbWarn_ = nullptr;
    QPushButton* rbBtn_ = nullptr;
    QTimer* timer_ = nullptr;
    bool loading_ = false;
};
} // namespace hypeclip
