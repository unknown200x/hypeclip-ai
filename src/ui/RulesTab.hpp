#pragma once
#include "core/Types.hpp"
#include <QWidget>
#include <vector>
class QListWidget; class QLineEdit; class QCheckBox; class QComboBox;
class QTableWidget; class QLabel;

namespace hypeclip {

// Priority #7 — visual IF/THEN rule editor (Streamer.bot-style).
class RulesTab : public QWidget {
    Q_OBJECT
public:
    explicit RulesTab(QWidget* parent = nullptr);
private slots:
    void onSelect(int row);
    void addRule();
    void deleteRule();
    void addCondition();
    void removeCondition();
    void addExampleClutch();
    void addExampleScream();
    void uiChanged();
private:
    void rebuildList();
    void loadRule(int idx);
    void readUiIntoRule();
    void rebuildConditionTable(const Rule& r);
    void commit();

    std::vector<Rule> rules_;
    int current_ = -1;
    bool loading_ = false;

    QListWidget*  list_ = nullptr;
    QLineEdit*    name_ = nullptr;
    QCheckBox*    enabled_ = nullptr;
    QComboBox*    join_ = nullptr;
    QComboBox*    action_ = nullptr;
    QTableWidget* conds_ = nullptr;
    QLabel*       summary_ = nullptr;
};
} // namespace hypeclip
