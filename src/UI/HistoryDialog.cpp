#include "UI/HistoryDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

HistoryDialog::HistoryDialog(const QString &historyFilePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(historyFilePath)
{
    setupUi();
    loadHistory();
    refreshList();
}

void HistoryDialog::setupUi()
{
    setWindowTitle(QStringLiteral("计算历史记录"));
    resize(800, 550);
    setMinimumSize(600, 400);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // 顶部：记录数标签
    m_countLabel = new QLabel(QStringLiteral("共 0 条记录"), this);
    m_countLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #4a90d9;"));
    mainLayout->addWidget(m_countLabel);

    // 中间：左右分栏
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧：历史列表
    m_listWidget = new QListWidget(this);
    m_listWidget->setMinimumWidth(250);
    m_listWidget->setStyleSheet(
        QStringLiteral("QListWidget { border: 1px solid #d0d0d0; border-radius: 4px; font-size: 11px; }"
                       "QListWidget::item { padding: 8px 6px; border-bottom: 1px solid #f0f0f0; }"
                       "QListWidget::item:selected { background-color: #e0e8f5; color: #2d6bb3; }"
                       "QListWidget::item:hover { background-color: #f5f6fa; }"));
    splitter->addWidget(m_listWidget);

    // 右侧：详情面板
    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto *detailLabel = new QLabel(QStringLiteral("计算详情"), this);
    detailLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 12px; color: #555555;"));
    rightLayout->addWidget(detailLabel);

    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setStyleSheet(
        QStringLiteral("QTextEdit { border: 1px solid #d0d0d0; border-radius: 4px; "
                       "background-color: #fafafa; font-size: 11px; padding: 8px; }"));
    rightLayout->addWidget(m_detailView);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter, 1);

    // 底部按钮栏
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_deleteBtn = new QPushButton(QStringLiteral("删除选中"), this);
    m_deleteBtn->setMinimumHeight(30);
    m_deleteBtn->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #e8a838; color: white; border: none; "
                       "border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
                       "QPushButton:hover { background-color: #d4952a; }"
                       "QPushButton:disabled { background-color: #d0d0d0; color: #808080; }"));
    btnLayout->addWidget(m_deleteBtn);

    m_clearBtn = new QPushButton(QStringLiteral("清除全部历史"), this);
    m_clearBtn->setMinimumHeight(30);
    m_clearBtn->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #e05555; color: white; border: none; "
                       "border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
                       "QPushButton:hover { background-color: #c94444; }"
                       "QPushButton:disabled { background-color: #d0d0d0; color: #808080; }"));
    btnLayout->addWidget(m_clearBtn);

    btnLayout->addStretch();

    m_closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    m_closeBtn->setMinimumHeight(30);
    m_closeBtn->setMinimumWidth(80);
    m_closeBtn->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #4a90d9; color: white; border: none; "
                       "border-radius: 4px; padding: 4px 16px; font-size: 12px; }"
                       "QPushButton:hover { background-color: #3a7bc8; }"));
    btnLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(btnLayout);

    // 信号连接
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &HistoryDialog::onItemSelected);
    connect(m_clearBtn, &QPushButton::clicked, this, &HistoryDialog::onClearHistory);
    connect(m_deleteBtn, &QPushButton::clicked, this, &HistoryDialog::onDeleteSelected);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    // 初始状态
    m_deleteBtn->setEnabled(false);
    m_clearBtn->setEnabled(false);
    m_detailView->setPlaceholderText(QStringLiteral("请选择左侧的历史记录查看详情"));
}

void HistoryDialog::loadHistory()
{
    m_history.clear();

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        if (val.isObject()) {
            m_history.append(CalculationHistoryRecord::fromJson(val.toObject()));
        }
    }
}

void HistoryDialog::saveHistory()
{
    QJsonArray arr;
    for (const CalculationHistoryRecord &h : m_history) {
        arr.append(h.toJson());
    }

    QJsonDocument doc(arr);
    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void HistoryDialog::refreshList()
{
    m_listWidget->blockSignals(true);
    m_listWidget->clear();

    // 按时间倒序显示（最新的在前）
    for (int i = m_history.size() - 1; i >= 0; --i) {
        const CalculationHistoryRecord &rec = m_history[i];
        QString label = rec.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QString mode = rec.calculationMode;
        QString customer = rec.selectedCustomer.isEmpty()
            ? QStringLiteral("全部") : rec.selectedCustomer;
        label += QStringLiteral("  |  %1  |  %2").arg(mode, customer);
        label += QStringLiteral("  |  %1条/%2成功").arg(rec.totalCount).arg(rec.successCount);

        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, i);  // 存储在原始列表中的索引
        item->setToolTip(label);
        m_listWidget->addItem(item);
    }

    m_listWidget->blockSignals(false);

    bool hasRecords = !m_history.isEmpty();
    m_countLabel->setText(QStringLiteral("共 %1 条记录").arg(m_history.size()));
    m_clearBtn->setEnabled(hasRecords);
    m_deleteBtn->setEnabled(false);
    m_detailView->clear();
}

void HistoryDialog::onItemSelected(int row)
{
    if (row < 0 || row >= m_listWidget->count()) {
        m_deleteBtn->setEnabled(false);
        m_detailView->clear();
        return;
    }

    QListWidgetItem *item = m_listWidget->item(row);
    int originalIndex = item->data(Qt::UserRole).toInt();

    if (originalIndex >= 0 && originalIndex < m_history.size()) {
        m_detailView->setHtml(formatRecordDetail(m_history[originalIndex]));
        m_deleteBtn->setEnabled(true);
    }
}

QString HistoryDialog::formatRecordDetail(const CalculationHistoryRecord &rec) const
{
    QString html;
    html += QStringLiteral("<h3 style='color:#4a90d9; margin-bottom:8px;'>计算记录</h3>");

    // 基本信息
    html += QStringLiteral("<table style='width:100%; border-collapse:collapse; font-size:11px;'>");

    auto addRow = [&](const QString &label, const QString &value) {
        html += QStringLiteral("<tr><td style='padding:4px 8px; color:#888; white-space:nowrap; "
                               "vertical-align:top;'>%1</td>"
                               "<td style='padding:4px 8px; color:#333;'>%2</td></tr>")
                    .arg(label, value);
    };

    addRow(QStringLiteral("计算时间"),
           rec.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    addRow(QStringLiteral("计算模式"), rec.calculationMode);
    addRow(QStringLiteral("选择客户"),
           rec.selectedCustomer.isEmpty() ? QStringLiteral("全部") : rec.selectedCustomer);
    addRow(QStringLiteral("线程数"), QString::number(rec.threadCount));

    // 导入文件
    QString filesHtml;
    for (const QString &f : rec.fileNames) {
        if (!filesHtml.isEmpty()) filesHtml += QStringLiteral("<br>");
        filesHtml += QFileInfo(f).fileName();
    }
    addRow(QStringLiteral("导入文件"), filesHtml.isEmpty() ? QStringLiteral("-") : filesHtml);

    html += QStringLiteral("</table>");

    // 计算结果
    html += QStringLiteral("<h4 style='color:#555; margin:12px 0 6px 0;'>计算结果</h4>");
    html += QStringLiteral("<table style='width:100%; border-collapse:collapse; font-size:11px;'>");
    addRow(QStringLiteral("总记录数"), QString::number(rec.totalCount));
    addRow(QStringLiteral("成功数"),
           QStringLiteral("<span style='color:#27ae60; font-weight:bold;'>%1</span>")
               .arg(rec.successCount));
    addRow(QStringLiteral("失败数"),
           rec.errorCount > 0
               ? QStringLiteral("<span style='color:#e74c3c; font-weight:bold;'>%1</span>")
                     .arg(rec.errorCount)
               : QStringLiteral("0"));
    addRow(QStringLiteral("总运费"),
           QStringLiteral("<span style='color:#4a90d9; font-weight:bold;'>%1 元</span>")
               .arg(rec.totalFreight, 0, 'f', 2));
    html += QStringLiteral("</table>");

    // 使用的规则
    html += QStringLiteral("<h4 style='color:#555; margin:12px 0 6px 0;'>使用规则</h4>");
    html += QStringLiteral("<table style='width:100%; border-collapse:collapse; font-size:11px;'>");
    QString rulesStr = rec.ruleNames.isEmpty()
        ? QStringLiteral("<span style='color:#999;'>默认报价表</span>")
        : rec.ruleNames.join(QStringLiteral("、"));
    addRow(QStringLiteral("客户规则"), rulesStr);
    addRow(QStringLiteral("活动加价规则数"), QString::number(rec.activityRuleCount));
    addRow(QStringLiteral("临时加价规则数"), QString::number(rec.tempPriceIncreaseCount));
    addRow(QStringLiteral("省份加价规则数"), QString::number(rec.provincePriceIncreaseCount));
    html += QStringLiteral("</table>");

    // 拉均重参数
    html += QStringLiteral("<h4 style='color:#555; margin:12px 0 6px 0;'>拉均重参数</h4>");
    html += QStringLiteral("<table style='width:100%; border-collapse:collapse; font-size:11px;'>");
    addRow(QStringLiteral("均重基准"), QStringLiteral("%1 kg").arg(rec.avgWeightBase, 0, 'f', 2));
    addRow(QStringLiteral("均重上限"), QStringLiteral("%1 kg").arg(rec.avgWeightUpperLimit, 0, 'f', 2));
    addRow(QStringLiteral("均重步长"), QStringLiteral("%1 kg").arg(rec.avgWeightIncrement, 0, 'f', 2));
    addRow(QStringLiteral("均重加价"), QStringLiteral("%1 元").arg(rec.avgWeightSurcharge, 0, 'f', 2));
    html += QStringLiteral("</table>");

    return html;
}

void HistoryDialog::onDeleteSelected()
{
    int currentRow = m_listWidget->currentRow();
    if (currentRow < 0) return;

    QListWidgetItem *item = m_listWidget->item(currentRow);
    int originalIndex = item->data(Qt::UserRole).toInt();

    if (originalIndex >= 0 && originalIndex < m_history.size()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, QStringLiteral("确认删除"),
            QStringLiteral("确定要删除这条历史记录吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (reply != QMessageBox::Yes) return;

        m_history.removeAt(originalIndex);
        saveHistory();
        refreshList();
    }
}

void HistoryDialog::onClearHistory()
{
    if (m_history.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, QStringLiteral("确认清除"),
        QStringLiteral("确定要清除全部 %1 条历史记录吗？\n此操作不可恢复。")
            .arg(m_history.size()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    m_history.clear();
    saveHistory();
    refreshList();
}
