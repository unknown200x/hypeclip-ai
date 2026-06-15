#pragma once
// Highlights + timeline list: every saved clip with type, confidence and time.
// Double-click opens the clip in the system player. Subscribes to ClipRecord
// events to append live, marshalling onto the UI thread.
#include <QWidget>

class QListWidget;
class QListWidgetItem;

namespace hypeclip {

class HighlightsTab : public QWidget {
    Q_OBJECT
public:
    explicit HighlightsTab(QWidget* parent = nullptr);
    ~HighlightsTab() override;

signals:
    void clipArrived(QString title, QString path, int confidence);

private slots:
    void appendClip(QString title, QString path, int confidence);
    void open(QListWidgetItem* item);

private:
    QListWidget* list_ = nullptr;
    unsigned long long busToken_ = 0;
};

} // namespace hypeclip
