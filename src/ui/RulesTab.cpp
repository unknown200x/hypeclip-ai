#include "ui/RulesTab.hpp"
#include "core/Config.hpp"
#include "core/PipelineController.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>

namespace hypeclip {

static const Metric kMetrics[] = {
    Metric::MicExcitement, Metric::MicVolume, Metric::MicPeak, Metric::ScreamScore,
    Metric::LaughterScore, Metric::ReactionScore, Metric::GameSpike, Metric::GameIntensity,
    Metric::GameLevel, Metric::VisionScore, Metric::KillFeed, Metric::Momentum, Metric::Confidence };

static QComboBox* metricCombo(Metric sel) {
    auto* c = new QComboBox();
    for (Metric m : kMetrics) c->addItem(to_string(m));
    for (int i = 0; i < (int)(sizeof(kMetrics)/sizeof(Metric)); ++i) if (kMetrics[i]==sel) c->setCurrentIndex(i);
    return c;
}
static QComboBox* opCombo(Comparator sel) {
    auto* c = new QComboBox();
    c->addItem(">"); c->addItem(">="); c->addItem("<"); c->addItem("<=");
    c->setCurrentIndex((int)sel);
    return c;
}

RulesTab::RulesTab(QWidget* parent) : QWidget(parent) {
    rules_ = Config::instance().get().rules;
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12,12,12,12);

    auto* hint = new QLabel("Create IF / THEN rules. With no rules, the AI scoring engine decides.");
    hint->setStyleSheet("color:#9aa0aa;"); hint->setWordWrap(true);
    root->addWidget(hint);

    auto* split = new QHBoxLayout();
    // Left: rule list + add/delete.
    auto* leftCol = new QVBoxLayout();
    list_ = new QListWidget(); list_->setMaximumWidth(190);
    leftCol->addWidget(list_);
    auto* addBtn = new QPushButton("+ Rule"); auto* delBtn = new QPushButton("Delete"); delBtn->setObjectName("danger");
    leftCol->addWidget(addBtn); leftCol->addWidget(delBtn);
    auto* ex1 = new QPushButton("Example: Clutch"); auto* ex2 = new QPushButton("Example: Big Scream");
    leftCol->addWidget(ex1); leftCol->addWidget(ex2);
    leftCol->addStretch();
    split->addLayout(leftCol);

    // Right: editor.
    auto* form = new QFormLayout();
    name_ = new QLineEdit();
    enabled_ = new QCheckBox("Enabled");
    join_ = new QComboBox(); join_->addItem("Match ALL conditions (AND)"); join_->addItem("Match ANY condition (OR)");
    action_ = new QComboBox(); action_->addItem("Create Clip"); action_->addItem("Create Replay");
    form->addRow("Name", name_);
    form->addRow("", enabled_);
    form->addRow("Logic", join_);
    form->addRow("Then", action_);
    conds_ = new QTableWidget(0,3);
    conds_->setHorizontalHeaderLabels({"Metric","Op","Value"});
    conds_->horizontalHeader()->setStretchLastSection(true);
    conds_->verticalHeader()->setVisible(false);
    form->addRow("Conditions", conds_);
    auto* condBtns = new QHBoxLayout();
    auto* addCond = new QPushButton("+ Condition"); auto* rmCond = new QPushButton("- Condition");
    condBtns->addWidget(addCond); condBtns->addWidget(rmCond); condBtns->addStretch();
    form->addRow("", new QWidget());
    auto* rightCol = new QVBoxLayout();
    rightCol->addLayout(form); rightCol->addLayout(condBtns);
    summary_ = new QLabel(); summary_->setStyleSheet("color:#39d98a;"); summary_->setWordWrap(true);
    rightCol->addWidget(summary_); rightCol->addStretch();
    split->addLayout(rightCol, 1);
    root->addLayout(split);

    connect(list_, &QListWidget::currentRowChanged, this, &RulesTab::onSelect);
    connect(addBtn, &QPushButton::clicked, this, &RulesTab::addRule);
    connect(delBtn, &QPushButton::clicked, this, &RulesTab::deleteRule);
    connect(addCond, &QPushButton::clicked, this, &RulesTab::addCondition);
    connect(rmCond, &QPushButton::clicked, this, &RulesTab::removeCondition);
    connect(ex1, &QPushButton::clicked, this, &RulesTab::addExampleClutch);
    connect(ex2, &QPushButton::clicked, this, &RulesTab::addExampleScream);
    connect(name_, &QLineEdit::textEdited, this, &RulesTab::uiChanged);
    connect(enabled_, &QCheckBox::toggled, this, &RulesTab::uiChanged);
    connect(join_, qOverload<int>(&QComboBox::currentIndexChanged), this, &RulesTab::uiChanged);
    connect(action_, qOverload<int>(&QComboBox::currentIndexChanged), this, &RulesTab::uiChanged);

    rebuildList();
    if (!rules_.empty()) list_->setCurrentRow(0);
}

void RulesTab::rebuildList() {
    loading_ = true;
    list_->clear();
    for (const auto& r : rules_)
        list_->addItem(QString(r.enabled ? "[on]  " : "[off] ") + QString::fromStdString(r.name));
    loading_ = false;
}

void RulesTab::onSelect(int row) { if (row >= 0 && row < (int)rules_.size()) loadRule(row); }

void RulesTab::loadRule(int idx) {
    current_ = idx; loading_ = true;
    const Rule& r = rules_[idx];
    name_->setText(QString::fromStdString(r.name));
    enabled_->setChecked(r.enabled);
    join_->setCurrentIndex(r.join == RuleJoin::Any ? 1 : 0);
    action_->setCurrentIndex(r.action == RuleAction::CreateReplay ? 1 : 0);
    rebuildConditionTable(r);
    loading_ = false;
    uiChanged();   // refresh summary
}

void RulesTab::rebuildConditionTable(const Rule& r) {
    conds_->setRowCount(0);
    for (const auto& c : r.conditions) {
        int row = conds_->rowCount(); conds_->insertRow(row);
        auto* mc = metricCombo(c.metric); auto* oc = opCombo(c.op);
        auto* vs = new QDoubleSpinBox(); vs->setRange(0,100); vs->setValue(c.value);
        conds_->setCellWidget(row,0,mc); conds_->setCellWidget(row,1,oc); conds_->setCellWidget(row,2,vs);
        connect(mc, qOverload<int>(&QComboBox::currentIndexChanged), this, &RulesTab::uiChanged);
        connect(oc, qOverload<int>(&QComboBox::currentIndexChanged), this, &RulesTab::uiChanged);
        connect(vs, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RulesTab::uiChanged);
    }
}

void RulesTab::readUiIntoRule() {
    if (current_ < 0 || current_ >= (int)rules_.size()) return;
    Rule& r = rules_[current_];
    r.name = name_->text().toStdString();
    r.enabled = enabled_->isChecked();
    r.join = join_->currentIndex()==1 ? RuleJoin::Any : RuleJoin::All;
    r.action = action_->currentIndex()==1 ? RuleAction::CreateReplay : RuleAction::CreateClip;
    r.conditions.clear();
    for (int row = 0; row < conds_->rowCount(); ++row) {
        auto* mc = qobject_cast<QComboBox*>(conds_->cellWidget(row,0));
        auto* oc = qobject_cast<QComboBox*>(conds_->cellWidget(row,1));
        auto* vs = qobject_cast<QDoubleSpinBox*>(conds_->cellWidget(row,2));
        if (!mc || !oc || !vs) continue;
        RuleCondition c; c.metric = kMetrics[mc->currentIndex()];
        c.op = (Comparator)oc->currentIndex(); c.value = (float)vs->value();
        r.conditions.push_back(c);
    }
}

void RulesTab::uiChanged() {
    if (loading_) return;
    readUiIntoRule();
    if (current_ >= 0 && current_ < (int)rules_.size()) {
        // refresh list label without losing selection
        loading_ = true;
        if (auto* it = list_->item(current_))
            it->setText(QString(rules_[current_].enabled?"[on]  ":"[off] ") + QString::fromStdString(rules_[current_].name));
        loading_ = false;
        const Rule& r = rules_[current_];
        QString sum = "IF ";
        for (size_t i=0;i<r.conditions.size();++i){ const auto& c=r.conditions[i];
            if (i) sum += (r.join==RuleJoin::All?" AND ":" OR ");
            sum += QString("%1 %2 %3").arg(QString::fromUtf8(to_string(c.metric)),
                                           QString::fromUtf8(to_string(c.op))).arg((int)c.value); }
        sum += r.action==RuleAction::CreateReplay ? "  ->  CREATE REPLAY" : "  ->  CREATE CLIP";
        summary_->setText(sum);
    }
    commit();
}

void RulesTab::addRule() {
    Rule r; r.name = "Rule " + std::to_string(rules_.size()+1);
    r.conditions.push_back({Metric::MicExcitement, Comparator::Greater, 80});
    rules_.push_back(r); rebuildList(); list_->setCurrentRow((int)rules_.size()-1); commit();
}
void RulesTab::deleteRule() {
    if (current_ < 0 || current_ >= (int)rules_.size()) return;
    rules_.erase(rules_.begin()+current_); current_ = -1;
    rebuildList(); if (!rules_.empty()) list_->setCurrentRow(0); commit();
}
void RulesTab::addCondition() {
    if (current_ < 0) return;
    rules_[current_].conditions.push_back({Metric::GameSpike, Comparator::Greater, 60});
    rebuildConditionTable(rules_[current_]); uiChanged();
}
void RulesTab::removeCondition() {
    if (current_ < 0 || rules_[current_].conditions.empty()) return;
    rules_[current_].conditions.pop_back();
    rebuildConditionTable(rules_[current_]); uiChanged();
}
void RulesTab::addExampleClutch() {
    Rule r; r.name="Insane Clutch"; r.join=RuleJoin::All; r.action=RuleAction::CreateReplay;
    r.conditions={{Metric::MicExcitement,Comparator::Greater,50},{Metric::GameSpike,Comparator::Greater,60}};
    rules_.push_back(r); rebuildList(); list_->setCurrentRow((int)rules_.size()-1); commit();
}
void RulesTab::addExampleScream() {
    Rule r; r.name="Big Scream"; r.join=RuleJoin::Any; r.action=RuleAction::CreateClip;
    r.conditions={{Metric::ScreamScore,Comparator::GreaterEqual,80},{Metric::MicVolume,Comparator::Greater,90}};
    rules_.push_back(r); rebuildList(); list_->setCurrentRow((int)rules_.size()-1); commit();
}

void RulesTab::commit() {
    Settings s = Config::instance().get();
    s.rules = rules_;
    Config::instance().set(s);
    PipelineController::instance().reconfigure();
}

} // namespace hypeclip
