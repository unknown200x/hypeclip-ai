#pragma once
// Priority #10 — replay/clip timeline with reason + actions.
#include "core/Types.hpp"
#include <QWidget>
#include <vector>
class QListWidget; class QListWidgetItem; class QLabel;

namespace hypeclip {

class HighlightsTab : public QWidget {
    Q_OBJECT
public:
    explicit HighlightsTab(QWidget* parent = nullptr);
    ~HighlightsTab() override;
signals:
    void clipArrived();
private slots:
    void refreshList();
    void showReason(int row);
    void openSelected();
    void favoriteSelected();
    void deleteSelected();
    void exportSelected();
private:
    QListWidget* list_ = nullptr;
    QLabel* reason_ = nullptr;
    std::vector<ClipRecord> clips_;
    unsigned long long busToken_ = 0;
};
} // namespace hypeclip
