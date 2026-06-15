#pragma once
// Custom-painted live "Hype Meter" / Clip-Worthiness gauge. Green→amber→red
// bar with a threshold tick. Polled by a QTimer from the dashboard.
#include <QWidget>

namespace hypeclip {

class HypeMeter : public QWidget {
    Q_OBJECT
public:
    explicit HypeMeter(QWidget* parent = nullptr);
    void setValue(float v01);          // 0..1
    void setThreshold(float v01);
    QSize sizeHint() const override { return QSize(220, 28); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    float value_ = 0.0f;
    float threshold_ = 0.70f;
};

} // namespace hypeclip
