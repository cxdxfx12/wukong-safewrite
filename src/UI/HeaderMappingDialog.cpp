#include "HeaderMappingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>

HeaderMappingDialog::HeaderMappingDialog(const QStringList &excelHeaders,
                                           const QMap<QString, int> &currentMapping,
                                           const QString &filePath,
                                           MappingTemplateManager *tplManager,
                                           QWidget *parent)
    : QDialog(parent)
    , m_headers(excelHeaders)
    , m_autoMapping(currentMapping)
    , m_defaultMapping(currentMapping)
    , m_currentMapping(currentMapping)
    , m_filePath(filePath)
    , m_tplManager(tplManager)
{
    setupUI();
    populatePreviewTable();
    populateMappingCombos();
    refreshTemplateCombo();
}

QMap<QString, int> HeaderMappingDialog::mapping() const
{
    return m_currentMapping;
}

QList<HeaderMappingDialog::FieldDef> HeaderMappingDialog::fieldDefs()
{
    return {
        {"运单号",   QStringLiteral("运单号"),   true},
        {"业务时间", QStringLiteral("业务时间"), true},
        {"订单客户", QStringLiteral("订单客户(店铺)"), true},
        {"客户",     QStringLiteral("结算对象(客户)"), true},
        {"目的省份", QStringLiteral("目的省份(运单送达地)"), true},
        {"实际重量", QStringLiteral("结算重量(实际重量)"), true},
        {"体积重",   QStringLiteral("体积重"),   false},
        {"运费",     QStringLiteral("运费"),     false},
        {"使用的规则", QStringLiteral("使用的规则"), false},
        {"错误信息", QStringLiteral("错误信息"), false},
    };
}

// ==================== UI 构建 ====================

void HeaderMappingDialog::setupUI()
{
    setWindowTitle(QStringLiteral("表头映射"));
    setMinimumSize(900, 560);
    resize(960, 600);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(10);

    // === 第一行：标题 + 文件路径 ===
    auto *titleRow = new QHBoxLayout();
    auto *titleLabel = new QLabel(QStringLiteral("Excel 表头映射"), this);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: bold; color: #2c3e50;"));
    titleRow->addWidget(titleLabel);
    titleRow->addStretch();
    auto *fileLabel = new QLabel(QStringLiteral("文件: %1").arg(m_filePath), this);
    fileLabel->setWordWrap(true);
    fileLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #7f8c9b;"));
    titleRow->addWidget(fileLabel);
    mainLayout->addLayout(titleRow);

    // === 第二行：模板选择器 ===
    auto *tplRow = new QHBoxLayout();
    tplRow->setSpacing(8);
    auto *tplLabel = new QLabel(QStringLiteral("映射模板:"), this);
    tplLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #5a6c7d;"));
    tplRow->addWidget(tplLabel);

    m_templateCombo = new QComboBox(this);
    m_templateCombo->setMinimumWidth(200);
    m_templateCombo->setFixedHeight(28);
    m_templateCombo->setToolTip(QStringLiteral("选择预设映射模板，自动匹配字段"));
    tplRow->addWidget(m_templateCombo);

    auto *saveTplBtn = new QPushButton(QStringLiteral("保存模板"), this);
    saveTplBtn->setObjectName(QStringLiteral("smallButton"));
    saveTplBtn->setFixedHeight(28);
    saveTplBtn->setToolTip(QStringLiteral("将当前映射保存为新模板"));
    tplRow->addWidget(saveTplBtn);

    auto *delTplBtn = new QPushButton(QStringLiteral("删除模板"), this);
    delTplBtn->setObjectName(QStringLiteral("smallButton"));
    delTplBtn->setFixedHeight(28);
    delTplBtn->setToolTip(QStringLiteral("删除选中的模板"));
    tplRow->addWidget(delTplBtn);

    tplRow->addStretch();
    mainLayout->addLayout(tplRow);

    // === 第三行：大表格（表头预览） ===
    auto *tableGroup = new QGroupBox(QStringLiteral("导入的 Excel 表头"), this);
    auto *tableLayout = new QVBoxLayout(tableGroup);
    tableLayout->setContentsMargins(10, 14, 10, 10);

    m_previewTable = new QTableWidget(0, 2, tableGroup);
    m_previewTable->setHorizontalHeaderLabels({
        QStringLiteral("列"),
        QStringLiteral("Excel 列名")
    });
    m_previewTable->horizontalHeader()->setStretchLastSection(true);
    m_previewTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_previewTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_previewTable->setColumnWidth(0, 55);
    m_previewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->verticalHeader()->setVisible(false);
    m_previewTable->setAlternatingRowColors(true);
    m_previewTable->setShowGrid(false);
    m_previewTable->verticalHeader()->setDefaultSectionSize(30);

    tableLayout->addWidget(m_previewTable);
    mainLayout->addWidget(tableGroup, 1);

    // === 第四行：关键字段映射（紧凑） ===
    auto *quickGroup = new QGroupBox(QStringLiteral("关键字段映射"), this);
    auto *quickLayout = new QHBoxLayout(quickGroup);
    quickLayout->setContentsMargins(14, 12, 14, 12);
    quickLayout->setSpacing(20);

    QList<FieldDef> fields = fieldDefs();
    for (const FieldDef &fd : fields) {
        if (!fd.required) continue;

        auto *combo = new QComboBox(quickGroup);
        combo->setMinimumWidth(120);
        combo->setFixedHeight(28);
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        combo->addItem(QStringLiteral("(不映射)"), -1);
        for (int i = 0; i < m_headers.size(); ++i) {
            QString text = m_headers[i];
            if (text.length() > 20)
                text = text.left(18) + QStringLiteral("..");
            combo->addItem(QStringLiteral("%1. %2").arg(QChar('A' + i)).arg(text), i);
        }

        auto *itemLayout = new QVBoxLayout();
        itemLayout->setSpacing(3);

        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(1);
        auto *label = new QLabel(fd.label, quickGroup);
        label->setStyleSheet(QStringLiteral("font-size: 11px; color: #5a6c7d; font-weight: bold;"));
        row->addWidget(label);
        // 关键字段红色星标
        auto *star = new QLabel(QStringLiteral("*"), quickGroup);
        star->setStyleSheet(QStringLiteral("color: #e74c3c; font-weight: bold; font-size: 11px;"));
        star->setFixedWidth(10);
        row->addWidget(star);
        itemLayout->addLayout(row);

        itemLayout->addWidget(combo);
        quickLayout->addLayout(itemLayout);

        m_comboMap[fd.key] = combo;
    }

    quickLayout->addStretch();
    mainLayout->addWidget(quickGroup);

    // === 底部按钮 ===
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    auto *resetBtn = new QPushButton(QStringLiteral("恢复默认"), this);
    resetBtn->setObjectName(QStringLiteral("smallButton"));
    resetBtn->setFixedHeight(30);

    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    cancelBtn->setObjectName(QStringLiteral("secondaryButton"));
    cancelBtn->setFixedHeight(32);

    auto *applyBtn = new QPushButton(QStringLiteral("应用并重新导入"), this);
    applyBtn->setObjectName(QStringLiteral("primaryButton"));
    applyBtn->setFixedHeight(32);

    bottomLayout->addWidget(resetBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(applyBtn);

    mainLayout->addLayout(bottomLayout);

    // 信号连接
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HeaderMappingDialog::onTemplateSelected);
    connect(saveTplBtn, &QPushButton::clicked, this, &HeaderMappingDialog::onSaveTemplate);
    connect(delTplBtn, &QPushButton::clicked, this, &HeaderMappingDialog::onDeleteTemplate);
    connect(resetBtn, &QPushButton::clicked, this, &HeaderMappingDialog::onResetDefault);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyBtn, &QPushButton::clicked, this, &HeaderMappingDialog::onApply);
}

// ==================== 表格填充 ====================

void HeaderMappingDialog::populatePreviewTable()
{
    m_previewTable->setRowCount(m_headers.size());
    for (int i = 0; i < m_headers.size(); ++i) {
        auto *colItem = new QTableWidgetItem(QStringLiteral("%1").arg(QChar('A' + i)));
        colItem->setTextAlignment(Qt::AlignCenter);
        colItem->setFlags(colItem->flags() & ~Qt::ItemIsEditable);
        m_previewTable->setItem(i, 0, colItem);

        auto *nameItem = new QTableWidgetItem(m_headers[i]);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_previewTable->setItem(i, 1, nameItem);
    }
}

// ==================== 下拉填充 ====================

void HeaderMappingDialog::populateMappingCombos()
{
    for (auto it = m_comboMap.constBegin(); it != m_comboMap.constEnd(); ++it) {
        QComboBox *combo = it.value();
        int colIdx = m_defaultMapping.value(it.key(), -1);
        int idx = combo->findData(colIdx);
        combo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
}

void HeaderMappingDialog::refreshTemplateCombo()
{
    m_templateCombo->blockSignals(true);
    m_templateCombo->clear();
    m_templateCombo->addItem(QStringLiteral("(自动检测 / 手动调整)"));
    QStringList names = m_tplManager->templateNames();
    for (const QString &name : names)
        m_templateCombo->addItem(name);
    m_templateCombo->setCurrentIndex(0);
    m_templateCombo->blockSignals(false);
}

// ==================== 槽函数 ====================

void HeaderMappingDialog::onTemplateSelected(int index)
{
    if (index <= 0) return;  // 0 = 自动检测

    QString name = m_templateCombo->itemText(index);
    MappingTemplate tpl = m_tplManager->get(name);
    if (tpl.name.isEmpty()) return;

    // 用模板对当前表头做映射
    QMap<QString, int> tplMapping = tpl.applyTo(m_headers);
    if (tplMapping.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("模板匹配"),
            QStringLiteral("模板「%1」的列名与当前 Excel 表头不匹配，请手动调整。").arg(name));
        return;
    }

    // 先恢复自动检测结果，再用模板覆盖（避免前一个模板的残留字段）
    m_defaultMapping = m_autoMapping;
    for (auto it = tplMapping.constBegin(); it != tplMapping.constEnd(); ++it)
        m_defaultMapping[it.key()] = it.value();

    populateMappingCombos();
}

void HeaderMappingDialog::onSaveTemplate()
{
    // 收集当前映射
    QMap<QString, QString> headerMap;
    QList<FieldDef> fields = fieldDefs();
    for (const FieldDef &fd : fields) {
        int colIdx = m_defaultMapping.value(fd.key, -1);
        if (colIdx >= 0 && colIdx < m_headers.size())
            headerMap[fd.key] = m_headers[colIdx];
        else
            headerMap[fd.key] = QString();
    }

    // 弹出输入框
    bool ok = false;
    QString tplName = QInputDialog::getText(this,
        QStringLiteral("保存映射模板"),
        QStringLiteral("模板名称:"),
        QLineEdit::Normal,
        QString(),
        &ok);

    if (!ok || tplName.trimmed().isEmpty()) return;
    tplName = tplName.trimmed();

    // 确认覆盖
    QStringList existing = m_tplManager->templateNames();
    if (existing.contains(tplName)) {
        auto ret = QMessageBox::question(this, QStringLiteral("覆盖模板"),
            QStringLiteral("模板「%1」已存在，是否覆盖？").arg(tplName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }

    // 构建并保存模板
    MappingTemplate tpl;
    tpl.name = tplName;
    tpl.courier = QString();  // 用户自定义模板不关联快递公司
    tpl.headerMapping = headerMap;
    m_tplManager->save(tpl);

    refreshTemplateCombo();
    // 选中刚保存的模板
    int idx = m_templateCombo->findText(tplName);
    if (idx >= 0) m_templateCombo->setCurrentIndex(idx);

    QMessageBox::information(this, QStringLiteral("保存成功"),
        QStringLiteral("映射模板「%1」已保存。").arg(tplName));
}

void HeaderMappingDialog::onDeleteTemplate()
{
    int index = m_templateCombo->currentIndex();
    if (index <= 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("请先选择要删除的模板。"));
        return;
    }

    QString name = m_templateCombo->itemText(index);
    auto ret = QMessageBox::question(this, QStringLiteral("删除模板"),
        QStringLiteral("确定要删除模板「%1」吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    m_tplManager->remove(name);
    refreshTemplateCombo();
}

void HeaderMappingDialog::onResetDefault()
{
    m_defaultMapping = m_autoMapping;
    populateMappingCombos();
}
void HeaderMappingDialog::onApply()
{
    // 从下拉读取关键字段映射
    QMap<QString, int> newMapping = m_defaultMapping;

    QList<FieldDef> fields = fieldDefs();
    for (const FieldDef &fd : fields) {
        if (!fd.required) continue;
        QComboBox *combo = m_comboMap.value(fd.key);
        if (!combo) continue;
        int colIdx = combo->currentData().toInt();
        if (colIdx >= 0) {
            newMapping[fd.key] = colIdx;
        } else {
            QMessageBox::warning(this, QStringLiteral("映射不完整"),
                QStringLiteral("关键字段「%1」必须映射到 Excel 中的一列。").arg(fd.label));
            return;
        }
    }

    m_currentMapping = newMapping;
    accept();
}
