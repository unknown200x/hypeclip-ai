#include "ui/HighlightsTab.hpp"
#include "core/EventBus.hpp"
#include "core/PipelineController.hpp"

#include <QVBoxLayout>
#include <QListWidget>
#include <QDesktopServices>
#include <QUrl>

namespace hypeclip {

HighlightsTab::HighlightsTab(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    list_ = new QListWidget();
    list_->setAlternatingRowColors(true);
    root->addWidget(list_);
    connect(list_, &QListWidget::itemDoubleClicked, this, &HighlightsTab::open);

    // Backfill any clips already in this session.
    for (const auto& c : PipelineController::instance().clips())
        appendClip(QString::fromStdString(c.title),
                   QString::fromStdString(c.filePath), (int)c.confidence);

    // Live updates (handler runs off-UI-thread -> queued signal hop).
    connect(this, &HighlightsTab::clipArrived, this, &HighlightsTab::appendClip,
            Qt::QueuedConnection);
    busToken_ = EventBus::instance().onClipSaved([this](const ClipRecord& r){
        emit clipArrived(QString::fromStdString(r.title),
                         QString::fromStdString(r.filePath), (int)r.confidence);
    });
}

HighlightsTab::~HighlightsTab() {
    if (busToken_) EventBus::instance().unsubscribe(busToken_);
}

void HighlightsTab::appendClip(QString title, QString path, int confidence) {
    auto* item = new QListWidgetItem(
        QString("%1   ·   %2%").arg(title).arg(confidence));
    item->setData(Qt::UserRole, path);
    list_->insertItem(0, item);   // newest on top
}

void HighlightsTab::open(QListWidgetItem* item) {
    const QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

} // namespace hypeclip
