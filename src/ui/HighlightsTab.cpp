#include "ui/HighlightsTab.hpp"
#include "core/EventBus.hpp"
#include "core/PipelineController.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <filesystem>

namespace hypeclip {

HighlightsTab::HighlightsTab(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8,8,8,8);
    list_ = new QListWidget(); list_->setAlternatingRowColors(true);
    root->addWidget(list_, 1);

    reason_ = new QLabel("Select a clip to see why it was created.");
    reason_->setWordWrap(true); reason_->setStyleSheet("color:#9aa0aa;padding:4px;");
    root->addWidget(reason_);

    auto* btns = new QHBoxLayout();
    auto* open = new QPushButton("Open"); open->setObjectName("primary");
    auto* fav = new QPushButton("★ Favorite");
    auto* exp = new QPushButton("Export");
    auto* del = new QPushButton("Delete"); del->setObjectName("danger");
    btns->addWidget(open); btns->addWidget(fav); btns->addWidget(exp); btns->addStretch(); btns->addWidget(del);
    root->addLayout(btns);

    connect(list_, &QListWidget::currentRowChanged, this, &HighlightsTab::showReason);
    connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*){ openSelected(); });
    connect(open, &QPushButton::clicked, this, &HighlightsTab::openSelected);
    connect(fav,  &QPushButton::clicked, this, &HighlightsTab::favoriteSelected);
    connect(exp,  &QPushButton::clicked, this, &HighlightsTab::exportSelected);
    connect(del,  &QPushButton::clicked, this, &HighlightsTab::deleteSelected);

    connect(this, &HighlightsTab::clipArrived, this, &HighlightsTab::refreshList, Qt::QueuedConnection);
    busToken_ = EventBus::instance().onClipSaved([this](const ClipRecord&){ emit clipArrived(); });
    refreshList();
}
HighlightsTab::~HighlightsTab() { if (busToken_) EventBus::instance().unsubscribe(busToken_); }

void HighlightsTab::refreshList() {
    clips_ = PipelineController::instance().clips();
    const int keep = list_->currentRow();
    list_->clear();
    for (auto it = clips_.rbegin(); it != clips_.rend(); ++it) {
        const auto& c = *it;
        QString star = c.favorite ? "* " : "";
        list_->addItem(QString("%1%2   ·   %3%   ·   %4")
            .arg(star, QString::fromStdString(c.title)).arg((int)c.confidence)
            .arg(QString::fromStdString(c.reason.firedRuleName.empty() ? "AI" : c.reason.firedRuleName)));
    }
    if (keep >= 0 && keep < list_->count()) list_->setCurrentRow(keep);
}

// list shows newest-first, so map row -> clips_ index.
static int idxFor(int row, int n) { return n - 1 - row; }

void HighlightsTab::showReason(int row) {
    int i = idxFor(row, (int)clips_.size());
    if (i < 0 || i >= (int)clips_.size()) { reason_->setText(""); return; }
    const auto& r = clips_[i].reason;
    reason_->setText(QString("Why: %1\nAudio %2 · Voice %3 · Vision %4 · Momentum %5 · Final %6%")
        .arg(QString::fromStdString(r.explanation))
        .arg((int)r.audioScore).arg((int)r.voiceScore).arg((int)r.visionScore)
        .arg((int)r.momentum).arg((int)r.finalScore));
}

void HighlightsTab::openSelected() {
    int i = idxFor(list_->currentRow(), (int)clips_.size());
    if (i < 0 || i >= (int)clips_.size()) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(clips_[i].filePath)));
}
void HighlightsTab::favoriteSelected() {
    int i = idxFor(list_->currentRow(), (int)clips_.size());
    if (i < 0 || i >= (int)clips_.size()) return;
    clips_[i].favorite = !clips_[i].favorite;   // session view; persisted index updates on next clip
    refreshList();
}
void HighlightsTab::exportSelected() {
    int i = idxFor(list_->currentRow(), (int)clips_.size());
    if (i < 0 || i >= (int)clips_.size()) return;
    QFileInfo fi(QString::fromStdString(clips_[i].filePath));
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
}
void HighlightsTab::deleteSelected() {
    int i = idxFor(list_->currentRow(), (int)clips_.size());
    if (i < 0 || i >= (int)clips_.size()) return;
    std::error_code ec; std::filesystem::remove(clips_[i].filePath, ec);
    refreshList();
}

} // namespace hypeclip
