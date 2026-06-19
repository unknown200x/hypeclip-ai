#pragma once
#include <QWidget>
#include <QMap>
#include <QString>
class QListWidget; class QSlider; class QSpinBox; class QLabel; class QTimer; class QFormLayout; class QShowEvent;

namespace hypeclip {

class AudioTab : public QWidget {
    Q_OBJECT
public:
    explicit AudioTab(QWidget* parent = nullptr);
protected:
    void showEvent(QShowEvent* e) override;
private slots:
    void pushToConfig();
    void poll();
private:
    void refreshSources();
    QSlider* addSlider(QFormLayout* form, const QString& key, const QString& label, int val);
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
