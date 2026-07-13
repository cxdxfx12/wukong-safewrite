#include "UI/ChartDialog.h"
#include "UI/ChartWidgets.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QDateTime>
#include <algorithm>

ChartDialog::ChartDialog(const QList<OrderData> &orders,
                         const QString &calcMode,
                         QWidget *parent)
    : QDialog(parent)
    , m_calcMode(calcMode)
{
    // 计算两种维度统计数据
    m_stats = computeCustomerStats(orders);
    m_storeStats = computeStoreStats(orders);

    // 按运费降序排序
    std::sort(m_stats.begin(), m_stats.end(), CustomerStats::sortByFreight);
    std::sort(m_storeStats.begin(), m_storeStats.end(), CustomerStats::sortByFreight);

    setupUi();
}

void ChartDialog::setupUi()
{
    setWindowTitle(QStringLiteral("计算统计图表"));
    resize(960, 680);
    setMinimumSize(800, 560);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(14, 14, 14, 14);

    // 标题
    auto *titleLabel = new QLabel(this);
    QString titleText = QStringLiteral("运费结算统计");
    if (!m_calcMode.isEmpty())
        titleText += QStringLiteral(" — %1模式").arg(m_calcMode);
    titleLabel->setText(titleText);
    titleLabel->setStyleSheet(
        QStringLiteral("font-size: 16px; font-weight: bold; color: #4a90d9;"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 概要统计卡片
    auto *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(10);

    int totalCount = 0;
    double totalFreight = 0.0;
    double totalWeight = 0.0;
    for (const CustomerStats &s : m_stats) {
        totalCount += s.shipmentCount;
        totalFreight += s.totalFreight;
        totalWeight += s.totalWeight;
    }
    double avgWeightAll = totalCount > 0 ? totalWeight / totalCount : 0.0;

    m_cardTotal = new StatCardWidget(
        QStringLiteral("总件数"), QString::number(totalCount),
        QColor("#4a90d9"), this);
    m_cardFreight = new StatCardWidget(
        QStringLiteral("总运费"), QStringLiteral("¥%1").arg(totalFreight, 0, 'f', 2),
        QColor("#27ae60"), this);
    m_cardAvgWeight = new StatCardWidget(
        QStringLiteral("平均计费重量"), QStringLiteral("%1kg").arg(avgWeightAll, 0, 'f', 2),
        QColor("#e8a838"), this);
    m_cardStores = new StatCardWidget(
        QStringLiteral("店铺数"), QString::number(m_storeStats.size()),
        QColor("#e74c3c"), this);

    cardsLayout->addWidget(m_cardTotal);
    cardsLayout->addWidget(m_cardFreight);
    cardsLayout->addWidget(m_cardAvgWeight);
    cardsLayout->addWidget(m_cardStores);
    mainLayout->addLayout(cardsLayout);

    // 标签页
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        QStringLiteral("QTabWidget::pane { border: 1px solid #d0d0d0; border-radius: 6px; "
                       "background: white; }"
                       "QTabBar::tab { padding: 8px 20px; font-size: 12px; "
                       "border: 1px solid #d0d0d0; border-bottom: none; "
                       "border-top-left-radius: 6px; border-top-right-radius: 6px; "
                       "margin-right: 2px; background: #f5f6fa; color: #666; }"
                       "QTabBar::tab:selected { background: white; color: #4a90d9; "
                       "font-weight: bold; border-bottom: 2px solid #4a90d9; }"
                       "QTabBar::tab:hover { background: #e8ecf5; }"));

    m_tabWidget->addTab(createOverviewTab(), QStringLiteral("📊 综合概览"));
    m_tabWidget->addTab(createCountTab(), QStringLiteral("📦 件数分布"));
    m_tabWidget->addTab(createFreightTab(), QStringLiteral("💰 运费统计"));
    m_tabWidget->addTab(createStoreTab(), QStringLiteral("🏪 店铺数据"));
    mainLayout->addWidget(m_tabWidget, 1);

    // 底部按钮
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    auto *exportBtn = new QPushButton(QStringLiteral("导出图表"), this);
    exportBtn->setMinimumHeight(32);
    exportBtn->setMinimumWidth(100);
    exportBtn->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #27ae60; color: white; border: none; "
                       "border-radius: 4px; padding: 6px 20px; font-size: 12px; }"
                       "QPushButton:hover { background-color: #219a52; }"));

    btnLayout->addWidget(exportBtn);
    btnLayout->addStretch();

    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    closeBtn->setMinimumHeight(32);
    closeBtn->setMinimumWidth(100);
    closeBtn->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #4a90d9; color: white; border: none; "
                       "border-radius: 4px; padding: 6px 20px; font-size: 12px; }"
                       "QPushButton:hover { background-color: #3a7bc8; }"));
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);

    connect(exportBtn, &QPushButton::clicked, this, &ChartDialog::onExportImage);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

// ============================================================================
// 综合概览 Tab — Top 10 结算客户运费排名
// ============================================================================
QWidget* ChartDialog::createOverviewTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    int topN = qMin(10, m_stats.size());
    QStringList categories;
    QList<double> freightValues;
    for (int i = 0; i < topN; ++i) {
        categories << m_stats[i].customerName;
        freightValues << m_stats[i].totalFreight;
    }

    auto *barChart = new BarChartWidget(page);
    barChart->setTitle(QStringLiteral("Top %1 结算客户运费排名").arg(topN));
    barChart->setData(categories, freightValues, QStringLiteral(" 元"),
                      BarChartWidget::Horizontal, BarChartWidget::ShowValue);
    barChart->setMinimumHeight(qMax(250, topN * 36 + 40));
    layout->addWidget(barChart);

    int totalC = 0;
    double totalF = 0.0;
    for (const CustomerStats &s : m_stats) {
        totalC += s.shipmentCount;
        totalF += s.totalFreight;
    }
    auto *hint = new QLabel(
        QStringLiteral("共 %1 个结算客户 | 总件数 %2 | 总运费 ¥%3")
            .arg(m_stats.size())
            .arg(totalC)
            .arg(totalF, 0, 'f', 2),
        page);
    hint->setStyleSheet(QStringLiteral("color: #999; font-size: 10px;"));
    hint->setAlignment(Qt::AlignCenter);
    layout->addWidget(hint);

    return page;
}

// ============================================================================
// 件数分布 Tab — 环形图 + 条形图（按结算客户）
// ============================================================================
QWidget* ChartDialog::createCountTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    QList<QPair<QString, double>> pieData;
    int totalCount = 0;
    for (const CustomerStats &s : m_stats)
        totalCount += s.shipmentCount;

    int topN = qMin(9, m_stats.size());
    int shownCount = 0;
    for (int i = 0; i < topN; ++i) {
        pieData.append({m_stats[i].customerName, (double)m_stats[i].shipmentCount});
        shownCount += m_stats[i].shipmentCount;
    }
    int otherCount = totalCount - shownCount;
    if (otherCount > 0 && m_stats.size() > topN) {
        pieData.append({QStringLiteral("其他"), (double)otherCount});
    }

    auto *donut = new DonutChartWidget(page);
    donut->setData(pieData, QStringLiteral("总件数\n%1").arg(totalCount));
    donut->setMinimumWidth(420);
    layout->addWidget(donut);

    auto *rightPanel = new QWidget(page);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    QStringList categories;
    QList<double> countValues;
    int showN = qMin(10, m_stats.size());
    for (int i = 0; i < showN; ++i) {
        categories << m_stats[i].customerName;
        countValues << (double)m_stats[i].shipmentCount;
    }

    auto *barChart = new BarChartWidget(rightPanel);
    barChart->setTitle(QStringLiteral("结算客户件数排名"));
    barChart->setData(categories, countValues, QStringLiteral(" 件"),
                      BarChartWidget::Horizontal, BarChartWidget::ShowValue);
    barChart->setMinimumHeight(qMax(220, showN * 36 + 40));
    rightLayout->addWidget(barChart);

    layout->addWidget(rightPanel);

    return page;
}

// ============================================================================
// 运费统计 Tab — 环形图 + 柱形图（按结算客户）
// ============================================================================
QWidget* ChartDialog::createFreightTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QHBoxLayout(page);
    layout->setSpacing(10);
    layout->setContentsMargins(12, 12, 12, 12);

    double totalFreight = 0.0;
    for (const CustomerStats &s : m_stats)
        totalFreight += s.totalFreight;

    QList<QPair<QString, double>> pieData;
    int topN = qMin(9, m_stats.size());
    double shownFreight = 0.0;
    for (int i = 0; i < topN; ++i) {
        pieData.append({m_stats[i].customerName, m_stats[i].totalFreight});
        shownFreight += m_stats[i].totalFreight;
    }
    double otherFreight = totalFreight - shownFreight;
    if (otherFreight > 0.01 && m_stats.size() > topN) {
        pieData.append({QStringLiteral("其他"), otherFreight});
    }

    auto *donut = new DonutChartWidget(page);
    donut->setData(pieData, QStringLiteral("总运费\n¥%1").arg(totalFreight, 0, 'f', 0));
    donut->setMinimumWidth(420);
    layout->addWidget(donut);

    auto *rightPanel = new QWidget(page);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    QStringList categories;
    QList<double> freightValues;
    int showN = qMin(10, m_stats.size());
    for (int i = 0; i < showN; ++i) {
        categories << m_stats[i].customerName;
        freightValues << m_stats[i].totalFreight;
    }

    auto *barChart = new BarChartWidget(rightPanel);
    barChart->setTitle(QStringLiteral("结算客户运费排名"));
    barChart->setData(categories, freightValues, QStringLiteral(" 元"),
                      BarChartWidget::Horizontal, BarChartWidget::ShowValue);
    barChart->setMinimumHeight(qMax(220, showN * 36 + 40));
    rightLayout->addWidget(barChart);

    double avgFreight = 0.0;
    int totalCount = 0;
    for (const CustomerStats &s : m_stats) {
        avgFreight += s.totalFreight;
        totalCount += s.shipmentCount;
    }
    avgFreight = totalCount > 0 ? avgFreight / totalCount : 0.0;

    auto *hint = new QLabel(
        QStringLiteral("平均单件运费: ¥%1").arg(avgFreight, 0, 'f', 2), rightPanel);
    hint->setStyleSheet(QStringLiteral("color: #888; font-size: 10px;"));
    hint->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(hint);

    layout->addWidget(rightPanel);

    return page;
}

// ============================================================================
// 🏪 店铺数据 Tab — 按订单客户(店铺)分组，展示各店铺关键指标
// ============================================================================
QWidget* ChartDialog::createStoreTab()
{
    auto *page = new QWidget(this);
    auto *scrollArea = new QScrollArea(page);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *inner = new QWidget();
    auto *layout = new QVBoxLayout(inner);
    layout->setSpacing(14);
    layout->setContentsMargins(12, 12, 12, 12);

    if (m_storeStats.isEmpty()) {
        auto *emptyLabel = new QLabel(QStringLiteral("暂无店铺数据"), inner);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet(QStringLiteral("color: #999; font-size: 14px; padding: 40px;"));
        layout->addWidget(emptyLabel);
        scrollArea->setWidget(inner);

        auto *pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->addWidget(scrollArea);
        return page;
    }

    int showN = qMin(15, m_storeStats.size());

    // --- 店铺概览卡片行 ---
    // 计算所有店铺汇总
    int totalShipments = 0;
    double totalFreightAll = 0.0;
    double totalWeightAll = 0.0;
    for (const CustomerStats &s : m_storeStats) {
        totalShipments += s.shipmentCount;
        totalFreightAll += s.totalFreight;
        totalWeightAll += s.totalWeight;
    }
    double overallAvgWeight = totalShipments > 0 ? totalWeightAll / totalShipments : 0.0;
    double overallAvgFreight = totalShipments > 0 ? totalFreightAll / totalShipments : 0.0;

    auto *storeCardsLayout = new QHBoxLayout();
    storeCardsLayout->setSpacing(10);

    auto *cardStoreCount = new StatCardWidget(
        QStringLiteral("店铺总数"), QString::number(m_storeStats.size()),
        QColor("#e74c3c"), inner);
    auto *cardStoreShipments = new StatCardWidget(
        QStringLiteral("店铺总件数"), QString::number(totalShipments),
        QColor("#4a90d9"), inner);
    auto *cardStoreAvgW = new StatCardWidget(
        QStringLiteral("店铺均重"), QStringLiteral("%1kg").arg(overallAvgWeight, 0, 'f', 2),
        QColor("#e8a838"), inner);
    auto *cardStoreAvgF = new StatCardWidget(
        QStringLiteral("店铺均费"), QStringLiteral("¥%1").arg(overallAvgFreight, 0, 'f', 2),
        QColor("#27ae60"), inner);

    storeCardsLayout->addWidget(cardStoreCount);
    storeCardsLayout->addWidget(cardStoreShipments);
    storeCardsLayout->addWidget(cardStoreAvgW);
    storeCardsLayout->addWidget(cardStoreAvgF);
    layout->addLayout(storeCardsLayout);

    // --- 第一行图表：件数占比(环形) + 件数排名(条形) ---
    {
        auto *row = new QHBoxLayout();
        row->setSpacing(10);

        // 环形图：各店铺件数分布
        QList<QPair<QString, double>> pieData;
        int pieTopN = qMin(7, m_storeStats.size());
        int shownShipments = 0;
        for (int i = 0; i < pieTopN; ++i) {
            pieData.append({m_storeStats[i].customerName, (double)m_storeStats[i].shipmentCount});
            shownShipments += m_storeStats[i].shipmentCount;
        }
        int otherShipments = totalShipments - shownShipments;
        if (otherShipments > 0 && m_storeStats.size() > pieTopN)
            pieData.append({QStringLiteral("其他"), (double)otherShipments});

        auto *donut = new DonutChartWidget(inner);
        donut->setData(pieData, QStringLiteral("%1家店铺\n%2件").arg(m_storeStats.size()).arg(totalShipments));
        donut->setMinimumWidth(360);
        row->addWidget(donut);

        // 条形图：件数排名
        auto *rightPanel = new QWidget(inner);
        auto *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(4);

        QStringList cats;
        QList<double> vals;
        int barN = qMin(10, m_storeStats.size());
        for (int i = 0; i < barN; ++i) {
            cats << m_storeStats[i].customerName;
            vals << (double)m_storeStats[i].shipmentCount;
        }

        auto *bar = new BarChartWidget(rightPanel);
        bar->setTitle(QStringLiteral("店铺件数排名"));
        bar->setData(cats, vals, QStringLiteral(" 件"),
                     BarChartWidget::Horizontal, BarChartWidget::ShowValue);
        bar->setMinimumHeight(qMax(200, barN * 34 + 40));
        rightLayout->addWidget(bar);

        row->addWidget(rightPanel);
        layout->addLayout(row);
    }

    // --- 第二行图表：平均计费重量 (柱形图) ---
    {
        QStringList cats;
        QList<double> vals;
        for (int i = 0; i < showN; ++i) {
            cats << m_storeStats[i].customerName;
            vals << m_storeStats[i].avgWeight();
        }

        auto *chart = new BarChartWidget(inner);
        chart->setTitle(QStringLiteral("各店铺平均计费重量 (kg)"));
        chart->setData(cats, vals, QStringLiteral(" kg"),
                       BarChartWidget::Vertical, BarChartWidget::ShowValue);
        chart->setMinimumHeight(260);
        layout->addWidget(chart);
    }

    // --- 第三行图表：平均运费 (柱形图) ---
    {
        QStringList cats;
        QList<double> vals;
        for (int i = 0; i < showN; ++i) {
            cats << m_storeStats[i].customerName;
            vals << m_storeStats[i].avgFreight();
        }

        auto *chart = new BarChartWidget(inner);
        chart->setTitle(QStringLiteral("各店铺平均单件运费 (元)"));
        chart->setData(cats, vals, QStringLiteral(" 元"),
                       BarChartWidget::Vertical, BarChartWidget::ShowValue);
        chart->setMinimumHeight(260);
        layout->addWidget(chart);
    }

    layout->addStretch();

    scrollArea->setWidget(inner);

    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(scrollArea);

    return page;
}

// ============================================================================
// 导出为图片
// ============================================================================
void ChartDialog::onExportImage()
{
    QString defaultName = QStringLiteral("运费统计图表_%1.png")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出图表"),
        defaultName,
        QStringLiteral("PNG图片 (*.png);;JPEG图片 (*.jpg)"),
        nullptr,
        QFileDialog::DontUseNativeDialog);

    if (filePath.isEmpty())
        return;

    QWidget *currentPage = m_tabWidget->currentWidget();
    if (!currentPage) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                             QStringLiteral("无法获取当前页面。"));
        return;
    }

    qreal dpr = devicePixelRatioF();
    QPixmap pixmap(currentPage->size());
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::white);
    currentPage->render(&pixmap);

    bool saved = pixmap.save(filePath);
    if (saved) {
        QMessageBox::information(this, QStringLiteral("导出成功"),
                                 QStringLiteral("图表已保存到:\n%1").arg(filePath));
    } else {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                             QStringLiteral("无法保存图片，请检查路径和权限。"));
    }
}
