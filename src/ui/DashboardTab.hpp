#pragma once
// Dashboard: live status, hype meter, clip-worthiness, replay-buffer banner,
// clips-today counter. Polls the PipelineController via a QTimer.
#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;

namespace hypeclip {

class HypeMeter;

class DashboardTab : public QWidget {
    Q_OBJECT
public:
    explicit DashboardTab(QWidget* parent = nullptr);

private slots:
    void poll();
    void enableReplayBuffer();

private:
    HypeMeter* hype_ = nullptr;
    HypeMeter* clipWorthiness_ = nullptr;
    QLabel*    status_ = nullptr;
    QLabel*    clipsToday_ = nullptr;
    QPushButton* enableRb_ = nullptr;
    QLabel*    rbWarning_ = nullptr;
    QTimer*    timer_ = nullptr;
};

} // namespace hypeclip
