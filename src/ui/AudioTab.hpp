#pragma once
#include <QWidget>
#include <QMap>
#include <QString>
class QListWidget; class QSlider; class QSpinBox; class QLabel; class QTimer;

namespace hypeclip {

class AudioTab : public QWidget {
    Q_OBJECT
public:
    explicit AudioTab(QWidget* parent = nullptr);
private slots:
    void pushToConfig();
    void poll();
private:
    QSlider* addSlider(class QFormLayout* form, const QString& key, const QString& label, int val);
    QListWidget* micList_ = nullptr;
    QListWidget* gameList_ = nullptr;
    QMap<QString, QSlider*> sliders_;
    QSpinBox* cooldown_ = nullptr;
    QLabel* micMeter_ = nullptr;
    QLabel* gameMeter_ = nullptr;
    QTimer* timer_ = nullptr;
    bool loading_ = false;
};
} // namespace hypeclip
