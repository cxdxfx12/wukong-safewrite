#include "GlobalRulesDialog.h"
#include "Calculation/RuleManager.h"
#include "DataModel/PriceTable.h"
#include "Utils/ThemeManager.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QHeaderView>
#include <QLabel>

// 通用的创建加价模式 Combo
static QComboBox* createModeCombo(QWidget *parent, IncreaseMode defaultMode = IncreaseMode::PerTicketFixed)
{
    auto *c = new QComboBox(parent);
    c->addItem(increaseModeLabel(IncreaseMode::PerTicketFixed),   static_cast<int>(IncreaseMode::PerTicketFixed));
    c->addItem(increaseModeLabel(IncreaseMode::PerTicketPercent), static_cast<int>(IncreaseMode::PerTicketPercent));
    c->addItem(increaseModeLabel(IncreaseMode::PerKg),            static_cast<int>(IncreaseMode::PerKg));
    int idx = c->findData(static_cast<int>(defaultMode));
    if (idx >= 0) c->setCurrentIndex(idx);
    return c;
}

// 通用的创建启用 Checkbox（居中）
static QWidget* createCheckWidget(QWidget *parent, bool checked)
{
    auto *w = new QWidget(parent);
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setAlignment(Qt::AlignCenter);
    auto *cb = new QCheckBox(w);
    cb->setChecked(checked);
    l->addWidget(cb);
    return w;
}

// 通用的读取 Checkbox 状态
static bool readCheckWidget(QWidget *w)
{
    if (!w) return false;
    auto *cb = w->findChild<QCheckBox*>();
    return cb ? cb->isChecked() : false;
}

GlobalRulesDialog::GlobalRulesDialog(RuleManager *ruleManager, QWidget *parent)
    : QDialog(parent)
    , m_ruleManager(ruleManager)
{
    setupUI();
    loadRules();
}

void GlobalRulesDialog::setupUI()
{
    setWindowTitle(QStringLiteral("全局规则管理"));
    setMinimumSize(860, 640);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ========== 基础设置 ==========
    auto *topGroup = new QGroupBox(QStringLiteral("基础设置"), this);
    auto *topLayout = new QFormLayout(topGroup);
    topLayout->setContentsMargins(12, 16, 12, 12);
    topLayout->setSpacing(8);

    m_noWeightPriceSpin = new QDoubleSpinBox(this);
    m_noWeightPriceSpin->setRange(0, 99999);
    m_noWeightPriceSpin->setDecimals(2);
    m_noWeightPriceSpin->setSuffix(QStringLiteral(" 元"));
    topLayout->addRow(QStringLiteral("无重量默认价格："), m_noWeightPriceSpin);

    m_avgBaseSpin = new QDoubleSpinBox(this);
    m_avgBaseSpin->setRange(0, 99999); m_avgBaseSpin->setDecimals(2);
    m_avgBaseSpin->setSuffix(QStringLiteral(" kg"));
    topLayout->addRow(QStringLiteral("均重基准："), m_avgBaseSpin);

    m_avgLimitSpin = new QDoubleSpinBox(this);
    m_avgLimitSpin->setRange(0, 99999); m_avgLimitSpin->setDecimals(2);
    m_avgLimitSpin->setSuffix(QStringLiteral(" kg"));
    topLayout->addRow(QStringLiteral("均重上限："), m_avgLimitSpin);

    m_avgStepSpin = new QDoubleSpinBox(this);
    m_avgStepSpin->setRange(0, 99999); m_avgStepSpin->setDecimals(2);
    m_avgStepSpin->setSuffix(QStringLiteral(" kg"));
    topLayout->addRow(QStringLiteral("均重步长："), m_avgStepSpin);

    m_avgSurchargeSpin = new QDoubleSpinBox(this);
    m_avgSurchargeSpin->setRange(0, 99999); m_avgSurchargeSpin->setDecimals(2);
    m_avgSurchargeSpin->setSuffix(QStringLiteral(" 元"));
    topLayout->addRow(QStringLiteral("均重加价："), m_avgSurchargeSpin);

    mainLayout->addWidget(topGroup);

    // ========== Tab 页 ==========
    m_tabWidget = new QTabWidget(this);

    // 列定义: 名称 | 开始时间 | 结束时间 | 加价数值 | 加价模式 | 启用
    auto createTimeTable = [](QTableWidget *&table, QPushButton *&addBtn, QPushButton *&removeBtn,
                               QWidget *parent, const QString &tabLabel) -> QWidget* {
        auto *tab = new QWidget(parent);
        auto *lay = new QVBoxLayout(tab);
        lay->setContentsMargins(8, 8, 8, 8);
        lay->setSpacing(8);

        auto *hint = new QLabel(QStringLiteral("※ 开始/结束日期对应表格的「业务日期」列，非当前日期，日期不在范围内则不生效"), tab);
        hint->setStyleSheet(QStringLiteral("color: #e74c3c; font-size: 11px; padding: 2px 0;"));
        hint->setWordWrap(true);
        lay->addWidget(hint);

        table = new QTableWidget(0, 6, tab);
        table->setHorizontalHeaderLabels({
            QStringLiteral("名称"), QStringLiteral("开始日期"),
            QStringLiteral("结束日期"), QStringLiteral("加价数值"),
            QStringLiteral("加价模式"), QStringLiteral("启用")});
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
        table->setColumnWidth(5, 50);
        table->verticalHeader()->setDefaultSectionSize(32);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);

        auto *btnLay = new QHBoxLayout();
        btnLay->setSpacing(8);
        addBtn = new QPushButton(QStringLiteral("添加行"), tab);
        addBtn->setObjectName(QStringLiteral("smallButton"));
        addBtn->setFixedHeight(30);
        removeBtn = new QPushButton(QStringLiteral("删除行"), tab);
        removeBtn->setObjectName(QStringLiteral("smallButton"));
        removeBtn->setFixedHeight(30);
        btnLay->addWidget(addBtn);
        btnLay->addWidget(removeBtn);
        btnLay->addStretch();

        lay->addWidget(table, 1);
        lay->addLayout(btnLay);
        return tab;
    };

    // Tab1: 活动加价
    m_tabWidget->addTab(
        createTimeTable(m_activityTable, m_addActivityBtn, m_removeActivityBtn,
                        m_tabWidget, QStringLiteral("活动加价")),
        QStringLiteral("活动加价(*)"));

    // Tab2: 临时加价
    m_tabWidget->addTab(
        createTimeTable(m_tempIncreaseTable, m_addTempBtn, m_removeTempBtn,
                        m_tabWidget, QStringLiteral("临时加价")),
        QStringLiteral("临时加价(*)"));

    // Tab3: 省份加价 — 省份 | 加价数值 | 加价模式 | 启用
    {
        auto *provTab = new QWidget(m_tabWidget);
        auto *provLay = new QVBoxLayout(provTab);
        provLay->setContentsMargins(8, 8, 8, 8);
        provLay->setSpacing(8);

        m_provinceIncreaseTable = new QTableWidget(0, 4, provTab);
        m_provinceIncreaseTable->setHorizontalHeaderLabels({
            QStringLiteral("省份"), QStringLiteral("加价数值"),
            QStringLiteral("加价模式"), QStringLiteral("启用")});
        m_provinceIncreaseTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_provinceIncreaseTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_provinceIncreaseTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_provinceIncreaseTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
        m_provinceIncreaseTable->setColumnWidth(3, 50);
        m_provinceIncreaseTable->verticalHeader()->setDefaultSectionSize(32);
        m_provinceIncreaseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_provinceIncreaseTable->setAlternatingRowColors(true);

        auto *provBtnLay = new QHBoxLayout();
        provBtnLay->setSpacing(8);
        m_addProvinceBtn = new QPushButton(QStringLiteral("添加行"), provTab);
        m_addProvinceBtn->setObjectName(QStringLiteral("smallButton"));
        m_addProvinceBtn->setFixedHeight(30);
        m_removeProvinceBtn = new QPushButton(QStringLiteral("删除行"), provTab);
        m_removeProvinceBtn->setObjectName(QStringLiteral("smallButton"));
        m_removeProvinceBtn->setFixedHeight(30);
        provBtnLay->addWidget(m_addProvinceBtn);
        provBtnLay->addWidget(m_removeProvinceBtn);
        provBtnLay->addStretch();

        provLay->addWidget(m_provinceIncreaseTable, 1);
        provLay->addLayout(provBtnLay);

        m_tabWidget->addTab(provTab, QStringLiteral("省份加价(*)"));
    }

    mainLayout->addWidget(m_tabWidget, 1);

    // ========== 主题切换 ==========
    auto *themeLayout = new QHBoxLayout();
    themeLayout->setSpacing(8);
    auto *themeLabel = new QLabel(QStringLiteral("界面风格："), this);
    auto *themeCombo = new QComboBox(this);
    themeCombo->addItems(ThemeManager::themeNames());
    themeCombo->setCurrentText(ThemeManager::currentTheme());
    themeCombo->setFixedWidth(160);
    themeLayout->addStretch();
    themeLayout->addWidget(themeLabel);
    themeLayout->addWidget(themeCombo);
    mainLayout->addLayout(themeLayout);

    connect(themeCombo, &QComboBox::currentTextChanged, [](const QString &name) {
        ThemeManager::setCurrentTheme(name);
        qApp->setStyleSheet(ThemeManager::themeQSS(name));
    });

    // ========== 底部按钮 ==========
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);
    m_saveBtn = new QPushButton(QStringLiteral("保存"), this);
    m_saveBtn->setObjectName(QStringLiteral("primaryButton"));
    m_saveBtn->setMinimumHeight(36); m_saveBtn->setFixedWidth(120);
    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setObjectName(QStringLiteral("secondaryButton"));
    m_cancelBtn->setMinimumHeight(36); m_cancelBtn->setFixedWidth(120);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_saveBtn);
    bottomLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(bottomLayout);

    // ========== 信号 ==========
    connect(m_saveBtn, &QPushButton::clicked, this, &GlobalRulesDialog::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &GlobalRulesDialog::onCancelClicked);

    // 活动加价: 添加/删除
    connect(m_addActivityBtn, &QPushButton::clicked, [this]() {
        int row = m_activityTable->rowCount();
        m_activityTable->insertRow(row);
        m_activityTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("新活动")));
        auto *s = new QDateTimeEdit(QDateTime::currentDateTime(), m_activityTable);
        s->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); s->setCalendarPopup(true);
        m_activityTable->setCellWidget(row, 1, s);
        auto *e = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7), m_activityTable);
        e->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); e->setCalendarPopup(true);
        m_activityTable->setCellWidget(row, 2, e);
        auto *v = new QDoubleSpinBox(m_activityTable);
        v->setRange(0, 99999); v->setDecimals(2); v->setValue(0.1);
        m_activityTable->setCellWidget(row, 3, v);
        m_activityTable->setCellWidget(row, 4, createModeCombo(m_activityTable, IncreaseMode::PerTicketPercent));
        m_activityTable->setCellWidget(row, 5, createCheckWidget(m_activityTable, true));
        m_activityTable->selectRow(row);
    });
    connect(m_removeActivityBtn, &QPushButton::clicked, [this]() {
        int row = m_activityTable->currentRow();
        if (row >= 0 && QMessageBox::question(this, QStringLiteral("确认删除"),
            QStringLiteral("确定要删除这条活动加价规则吗？")) == QMessageBox::Yes)
            m_activityTable->removeRow(row);
    });

    // 临时加价: 添加/删除
    connect(m_addTempBtn, &QPushButton::clicked, [this]() {
        int row = m_tempIncreaseTable->rowCount();
        m_tempIncreaseTable->insertRow(row);
        m_tempIncreaseTable->setItem(row, 0, new QTableWidgetItem(QStringLiteral("新规则")));
        auto *s = new QDateTimeEdit(QDateTime::currentDateTime(), m_tempIncreaseTable);
        s->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); s->setCalendarPopup(true);
        m_tempIncreaseTable->setCellWidget(row, 1, s);
        auto *e = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7), m_tempIncreaseTable);
        e->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); e->setCalendarPopup(true);
        m_tempIncreaseTable->setCellWidget(row, 2, e);
        auto *v = new QDoubleSpinBox(m_tempIncreaseTable);
        v->setRange(0, 99999); v->setDecimals(2); v->setValue(1.0);
        m_tempIncreaseTable->setCellWidget(row, 3, v);
        m_tempIncreaseTable->setCellWidget(row, 4, createModeCombo(m_tempIncreaseTable, IncreaseMode::PerTicketFixed));
        m_tempIncreaseTable->setCellWidget(row, 5, createCheckWidget(m_tempIncreaseTable, true));
        m_tempIncreaseTable->selectRow(row);
    });
    connect(m_removeTempBtn, &QPushButton::clicked, [this]() {
        int row = m_tempIncreaseTable->currentRow();
        if (row >= 0 && QMessageBox::question(this, QStringLiteral("确认删除"),
            QStringLiteral("确定要删除这条临时加价规则吗？")) == QMessageBox::Yes)
            m_tempIncreaseTable->removeRow(row);
    });

    // 省份加价: 添加/删除
    connect(m_addProvinceBtn, &QPushButton::clicked, [this]() {
        const int totalProvs = 31;
        if (m_provinceIncreaseTable->rowCount() >= totalProvs) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("所有省份都已添加。"));
            return;
        }
        QStringList allProvs = {
            QStringLiteral("江苏"),QStringLiteral("浙江"),QStringLiteral("安徽"),QStringLiteral("上海"),
            QStringLiteral("山东"),QStringLiteral("广东"),QStringLiteral("福建"),QStringLiteral("北京"),
            QStringLiteral("河南"),QStringLiteral("湖北"),QStringLiteral("湖南"),QStringLiteral("江西"),
            QStringLiteral("天津"),QStringLiteral("河北"),QStringLiteral("山西"),QStringLiteral("广西"),
            QStringLiteral("四川"),QStringLiteral("重庆"),QStringLiteral("陕西"),QStringLiteral("贵州"),
            QStringLiteral("辽宁"),QStringLiteral("吉林"),QStringLiteral("黑龙江"),QStringLiteral("云南"),
            QStringLiteral("海南"),QStringLiteral("甘肃"),QStringLiteral("青海"),QStringLiteral("内蒙古"),
            QStringLiteral("宁夏"),QStringLiteral("新疆"),QStringLiteral("西藏")
        };
        QSet<QString> existing;
        for (int i = 0; i < m_provinceIncreaseTable->rowCount(); ++i) {
            auto *combo = qobject_cast<QComboBox*>(m_provinceIncreaseTable->cellWidget(i, 0));
            if (combo) existing.insert(combo->currentText());
        }
        QString defaultProv;
        for (const QString &p : allProvs) {
            if (!existing.contains(p)) { defaultProv = p; break; }
        }
        int row = m_provinceIncreaseTable->rowCount();
        m_provinceIncreaseTable->insertRow(row);
        auto *provCombo = new QComboBox(m_provinceIncreaseTable);
        populateProvinceCombo(provCombo);
        if (!defaultProv.isEmpty()) { int idx = provCombo->findText(defaultProv); if (idx >= 0) provCombo->setCurrentIndex(idx); }
        m_provinceIncreaseTable->setCellWidget(row, 0, provCombo);
        auto *amountSpin = new QDoubleSpinBox(m_provinceIncreaseTable);
        amountSpin->setRange(0, 99999); amountSpin->setDecimals(2); amountSpin->setValue(1.0);
        m_provinceIncreaseTable->setCellWidget(row, 1, amountSpin);
        m_provinceIncreaseTable->setCellWidget(row, 2, createModeCombo(m_provinceIncreaseTable, IncreaseMode::PerTicketFixed));
        m_provinceIncreaseTable->setCellWidget(row, 3, createCheckWidget(m_provinceIncreaseTable, true));
        m_provinceIncreaseTable->selectRow(row);
    });
    connect(m_removeProvinceBtn, &QPushButton::clicked, [this]() {
        int row = m_provinceIncreaseTable->currentRow();
        if (row >= 0 && QMessageBox::question(this, QStringLiteral("确认删除"),
            QStringLiteral("确定要删除这条省份加价规则吗？")) == QMessageBox::Yes)
            m_provinceIncreaseTable->removeRow(row);
    });
}

// ==================== 加载 & 构建 ====================

void GlobalRulesDialog::loadRules()
{
    GlobalRules rules = m_ruleManager->globalRules();
    m_noWeightPriceSpin->setValue(rules.noWeightDefaultPrice);
    m_avgBaseSpin->setValue(rules.avgWeightBase);
    m_avgLimitSpin->setValue(rules.avgWeightUpperLimit);
    m_avgStepSpin->setValue(rules.avgWeightIncrement);
    m_avgSurchargeSpin->setValue(rules.avgWeightSurcharge);

    // 时间型规则加载
    auto loadTimeRules = [](QTableWidget *table, const QList<PriceIncreaseRule> &list) {
        table->setRowCount(0);
        for (const PriceIncreaseRule &r : list) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(r.name));
            auto *s = new QDateTimeEdit(r.startTime, table);
            s->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); s->setCalendarPopup(true);
            table->setCellWidget(row, 1, s);
            auto *e = new QDateTimeEdit(r.endTime, table);
            e->setDisplayFormat(QStringLiteral("yyyy-MM-dd")); e->setCalendarPopup(true);
            table->setCellWidget(row, 2, e);
            auto *v = new QDoubleSpinBox(table);
            v->setRange(0, 99999); v->setDecimals(2); v->setValue(r.amount);
            table->setCellWidget(row, 3, v);
            table->setCellWidget(row, 4, createModeCombo(table, r.mode));
            table->setCellWidget(row, 5, createCheckWidget(table, r.isActive));
        }
    };
    loadTimeRules(m_activityTable, rules.activityRules);
    loadTimeRules(m_tempIncreaseTable, rules.tempPriceIncreases);

    // 省份规则加载
    m_provinceIncreaseTable->setRowCount(0);
    for (const PriceIncreaseRule &r : rules.provincePriceIncreases) {
        int row = m_provinceIncreaseTable->rowCount();
        m_provinceIncreaseTable->insertRow(row);
        auto *pc = new QComboBox(m_provinceIncreaseTable);
        populateProvinceCombo(pc);
        int idx = pc->findText(r.province);
        if (idx >= 0) pc->setCurrentIndex(idx);
        m_provinceIncreaseTable->setCellWidget(row, 0, pc);
        auto *v = new QDoubleSpinBox(m_provinceIncreaseTable);
        v->setRange(0, 99999); v->setDecimals(2); v->setValue(r.amount);
        m_provinceIncreaseTable->setCellWidget(row, 1, v);
        m_provinceIncreaseTable->setCellWidget(row, 2, createModeCombo(m_provinceIncreaseTable, r.mode));
        m_provinceIncreaseTable->setCellWidget(row, 3, createCheckWidget(m_provinceIncreaseTable, r.isActive));
    }
}

GlobalRules GlobalRulesDialog::buildGlobalRules() const
{
    GlobalRules rules;
    rules.noWeightDefaultPrice = m_noWeightPriceSpin->value();
    rules.avgWeightBase = m_avgBaseSpin->value();
    rules.avgWeightUpperLimit = m_avgLimitSpin->value();
    rules.avgWeightIncrement = m_avgStepSpin->value();
    rules.avgWeightSurcharge = m_avgSurchargeSpin->value();

    // 统一构建函数
    auto buildTimeRules = [](const QTableWidget *table) -> QList<PriceIncreaseRule> {
        QList<PriceIncreaseRule> list;
        for (int i = 0; i < table->rowCount(); ++i) {
            PriceIncreaseRule r;
            if (table->item(i, 0)) r.name = table->item(i, 0)->text();
            auto *s = qobject_cast<QDateTimeEdit*>(table->cellWidget(i, 1));
            if (s) r.startTime = s->dateTime();
            auto *e = qobject_cast<QDateTimeEdit*>(table->cellWidget(i, 2));
            if (e) r.endTime = e->dateTime();
            auto *v = qobject_cast<QDoubleSpinBox*>(table->cellWidget(i, 3));
            if (v) r.amount = v->value();
            auto *m = qobject_cast<QComboBox*>(table->cellWidget(i, 4));
            if (m) r.mode = static_cast<IncreaseMode>(m->currentData().toInt());
            r.isActive = readCheckWidget(table->cellWidget(i, 5));
            list.append(r);
        }
        return list;
    };
    rules.activityRules = buildTimeRules(m_activityTable);
    rules.tempPriceIncreases = buildTimeRules(m_tempIncreaseTable);

    // 省份规则构建
    for (int i = 0; i < m_provinceIncreaseTable->rowCount(); ++i) {
        PriceIncreaseRule r;
        auto *pc = qobject_cast<QComboBox*>(m_provinceIncreaseTable->cellWidget(i, 0));
        if (pc) r.province = pc->currentText();
        auto *v = qobject_cast<QDoubleSpinBox*>(m_provinceIncreaseTable->cellWidget(i, 1));
        if (v) r.amount = v->value();
        auto *m = qobject_cast<QComboBox*>(m_provinceIncreaseTable->cellWidget(i, 2));
        if (m) r.mode = static_cast<IncreaseMode>(m->currentData().toInt());
        r.isActive = readCheckWidget(m_provinceIncreaseTable->cellWidget(i, 3));
        rules.provincePriceIncreases.append(r);
    }

    return rules;
}

void GlobalRulesDialog::populateProvinceCombo(QComboBox *combo) const
{
    combo->addItems({
        QStringLiteral("江苏"),QStringLiteral("浙江"),QStringLiteral("安徽"),QStringLiteral("上海"),
        QStringLiteral("山东"),QStringLiteral("广东"),QStringLiteral("福建"),QStringLiteral("北京"),
        QStringLiteral("河南"),QStringLiteral("湖北"),QStringLiteral("湖南"),QStringLiteral("江西"),
        QStringLiteral("天津"),QStringLiteral("河北"),QStringLiteral("山西"),QStringLiteral("广西"),
        QStringLiteral("四川"),QStringLiteral("重庆"),QStringLiteral("陕西"),QStringLiteral("贵州"),
        QStringLiteral("辽宁"),QStringLiteral("吉林"),QStringLiteral("黑龙江"),QStringLiteral("云南"),
        QStringLiteral("海南"),QStringLiteral("甘肃"),QStringLiteral("青海"),QStringLiteral("内蒙古"),
        QStringLiteral("宁夏"),QStringLiteral("新疆"),QStringLiteral("西藏")
    });
}

void GlobalRulesDialog::onSaveClicked()
{
    QSet<QString> provSet;
    for (int i = 0; i < m_provinceIncreaseTable->rowCount(); ++i) {
        auto *combo = qobject_cast<QComboBox*>(m_provinceIncreaseTable->cellWidget(i, 0));
        if (combo) {
            QString prov = combo->currentText();
            if (provSet.contains(prov)) {
                QMessageBox::warning(this, QStringLiteral("保存失败"),
                    QStringLiteral("省份加价存在重复省份：%1").arg(prov));
                m_provinceIncreaseTable->selectRow(i);
                return;
            }
            provSet.insert(prov);
        }
    }
    GlobalRules rules = buildGlobalRules();
    m_ruleManager->setGlobalRules(rules);
    if (m_ruleManager->saveToFile(RuleManager::defaultFilePath())) {
        QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("全局规则已更新并保存"));
    } else {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("全局规则已更新，但写入文件失败"));
    }
}

void GlobalRulesDialog::onCancelClicked()
{
    reject();
}
