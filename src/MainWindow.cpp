#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QElapsedTimer>
#include <QApplication>
#include <QtConcurrent>
#include <QThread>
#include <QDir>
#include <QGridLayout>
#include <QProgressBar>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QPainter>
#include <QDesktopServices>
#include <QUrl>
#include <memory>
#include <functional>
#include <xlsxdocument.h>

#include "Excel/ExcelEngine.h"
#include "Excel/ExcelImporter.h"
#include "Excel/ExcelExporter.h"
#include "UI/RuleManagerDialog.h"
#include "UI/QuickTestWidget.h"
#include "UI/GlobalRulesDialog.h"
#include "UI/RuleHelpDialog.h"
#include "UI/HistoryDialog.h"
#include "UI/ChartDialog.h"
#include "UI/HeaderMappingDialog.h"
#include "Calculation/SimdCalculator.h"
#include "Utils/Logger.h"
#include "Utils/ProvinceUtils.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_calculator(new FreightCalculator(this))
    , m_ruleManager(new RuleManager(this))
    , m_tplManager(new MappingTemplateManager)
    , m_importWatcher(new QFutureWatcher<void>(this))
    , m_exportWatcher(new QFutureWatcher<void>(this))
    , m_batchWatcher(new QFutureWatcher<void>(this))
    , m_totalFileCount(0)
    , m_processedFileCount(0)
{
    ui->setupUi(this);

    setupBanner();
    setupConnections();
    setupStatusBar();
    setupToolbarIcons();

    m_ruleManager->initDefaultPriceTable();
    m_ruleManager->loadFromFile(RuleManager::defaultFilePath());
    m_calculator->setDefaultPriceTable(m_ruleManager->defaultPriceTable());
    m_calculator->setGlobalRules(m_ruleManager->globalRules());
    updateCustomerCombo();

    // 同步快递公司选择框
    {
        QString current = m_ruleManager->courierName();
        int idx = ui->courierCombo->findText(current);
        if (idx >= 0) {
            ui->courierCombo->blockSignals(true);
            ui->courierCombo->setCurrentIndex(idx);
            ui->courierCombo->blockSignals(false);
        }
    }

    // 从全局规则加载拉均重参数到界面（阻塞信号，避免初始化时触发保存）
    GlobalRules gr = m_ruleManager->globalRules();
    ui->avgBaseSpin->blockSignals(true);
    ui->avgLimitSpin->blockSignals(true);
    ui->avgStepSpin->blockSignals(true);
    ui->avgSurchargeSpin->blockSignals(true);
    ui->avgBaseSpin->setValue(gr.avgWeightBase);
    ui->avgLimitSpin->setValue(gr.avgWeightUpperLimit);
    ui->avgStepSpin->setValue(gr.avgWeightIncrement);
    ui->avgSurchargeSpin->setValue(gr.avgWeightSurcharge);
    ui->avgBaseSpin->blockSignals(false);
    ui->avgLimitSpin->blockSignals(false);
    ui->avgStepSpin->blockSignals(false);
    ui->avgSurchargeSpin->blockSignals(false);

    ui->mainSplitter->setStretchFactor(0, 3);
    ui->mainSplitter->setStretchFactor(1, 1);

    // 线程数：0 = 自动（使用 CPU 逻辑核心数）
    ui->threadSpin->setToolTip(QStringLiteral(
        "选择[自动]将根据 CPU 核心数自动使用最大可用线程\n"
        "也可手动指定 1-64 的线程数"));

    // 初始化：拉均重参数默认隐藏
    onCalcModeChanged(ui->modeCombo->currentText());

    m_overlayWidget = new QWidget(ui->dataTableView->parentWidget());
    m_overlayWidget->setObjectName(QStringLiteral("overlayWidget"));

    auto *overlayLayout = new QVBoxLayout(m_overlayWidget);
    overlayLayout->setAlignment(Qt::AlignCenter);

    m_centerProgressLabel = new QLabel(m_overlayWidget);
    m_centerProgressLabel->setAlignment(Qt::AlignCenter);
    m_centerProgressLabel->setStyleSheet(
        QStringLiteral("font-size: 14px; font-weight: bold; color: #4a90d9;"));

    m_centerProgress = new QProgressBar(m_overlayWidget);
    m_centerProgress->setMinimumWidth(300);
    m_centerProgress->setMaximumWidth(500);
    m_centerProgress->setMinimumHeight(24);
    m_centerProgress->setTextVisible(true);
    m_centerProgress->setStyleSheet(
        QStringLiteral("QProgressBar { font-size: 12px; }"));

    overlayLayout->addStretch();
    overlayLayout->addWidget(m_centerProgressLabel, 0, Qt::AlignCenter);
    overlayLayout->addWidget(m_centerProgress, 0, Qt::AlignCenter);
    overlayLayout->addStretch();

    m_overlayWidget->setStyleSheet(
        QStringLiteral("background-color: rgba(255, 255, 255, 200);"));
    m_overlayWidget->hide();
}

MainWindow::~MainWindow()
{
    delete m_tplManager;
    delete ui;
}

void MainWindow::setupConnections()
{
    connect(ui->importBtn, &QPushButton::clicked, this, &MainWindow::onImportClicked);
    connect(ui->headerMappingBtn, &QPushButton::clicked, this, &MainWindow::onHeaderMappingClicked);
    connect(ui->calculateBtn, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
    connect(ui->exportBtn, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(ui->quickTestBtn, &QPushButton::clicked, this, &MainWindow::onQuickTestClicked);
    connect(ui->ruleManageBtn, &QPushButton::clicked, this, &MainWindow::onRuleManageClicked);
    connect(ui->globalRulesBtn, &QPushButton::clicked, this, &MainWindow::onGlobalRulesClicked);
    connect(ui->startCalcBtn, &QPushButton::clicked, this, &MainWindow::onCalculateClicked);
    connect(ui->ruleHelpBtn, &QPushButton::clicked, this, &MainWindow::onRuleHelpClicked);  // 新增：规则说明按钮
    connect(ui->historyBtn, &QPushButton::clicked, this, &MainWindow::onHistoryClicked);    // 新增：历史记录按钮
    connect(ui->chartBtn, &QPushButton::clicked, this, &MainWindow::onChartClicked);
    connect(ui->clearBtn, &QPushButton::clicked, this, &MainWindow::onClearClicked);      // 新增：生成图表按钮

    connect(m_calculator, &FreightCalculator::progressUpdated, this, &MainWindow::throttledProgress);
    connect(m_calculator, &FreightCalculator::calculationComplete, this, &MainWindow::onCalculationComplete);

    // ★ 不通过 watcher 触发 onImportFinished，避免与 invokeMethod 回调重复
    // onImportFinished 由 processFunc 末尾的 invokeMethod 统一触发
    connect(m_importWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        // 仅作兜底：如果 invokeMethod 回调未执行，手动触发
        if (!m_currentOrders.isEmpty() && !m_isCalculating) {
            onImportFinished();
        }
    });
    connect(m_exportWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onExportFinished);
    connect(m_batchWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onAllFilesProcessed);

    connect(ui->firstPageBtn, &QPushButton::clicked, this, &MainWindow::onFirstPage);
    connect(ui->prevPageBtn, &QPushButton::clicked, this, &MainWindow::onPrevPage);
    connect(ui->nextPageBtn, &QPushButton::clicked, this, &MainWindow::onNextPage);
    connect(ui->lastPageBtn, &QPushButton::clicked, this, &MainWindow::onLastPage);

    connect(this, &MainWindow::importProgress, this, [this](int percent) {
        throttledProgress(percent);
        updateCenterProgress(percent);
    });
    connect(this, &MainWindow::calcProgress, this, [this](int percent) {
        throttledProgress(percent);
        updateCenterProgress(percent);
    });

    connect(ui->modeCombo, &QComboBox::currentTextChanged, this, &MainWindow::onCalcModeChanged);

    // 拉均重参数变化时自动保存到全局规则和文件
    connect(ui->avgBaseSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onAvgParamChanged);
    connect(ui->avgLimitSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onAvgParamChanged);
    connect(ui->avgStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onAvgParamChanged);
    connect(ui->avgSurchargeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onAvgParamChanged);

    // 快递公司切换
    connect(ui->courierCombo, &QComboBox::currentTextChanged, this, &MainWindow::onCourierChanged);
}

void MainWindow::setupStyle()
{
    applyLightStyle();
}

void MainWindow::setupStatusBar()
{
    auto *statusLabel = new QLabel(QStringLiteral("就绪"), this);
    statusLabel->setObjectName(QStringLiteral("statusLabel"));

    auto *companyBtn = new QPushButton(QStringLiteral("杭州喵喵至家网络有限公司"), this);
    companyBtn->setObjectName(QStringLiteral("companyLink"));
    companyBtn->setFlat(true);
    companyBtn->setCursor(Qt::PointingHandCursor);
    companyBtn->setStyleSheet(QStringLiteral(
        "QPushButton { color: #999999; font-size: 10px; border: none; background: transparent; }"
        "QPushButton:hover { color: #2980b9; }"));
    connect(companyBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.hbdxm.com/")));
    });

    auto *recordLabel = new QLabel(QStringLiteral("记录数: 0"), this);
    recordLabel->setObjectName(QStringLiteral("recordLabel"));

    auto *timeLabel = new QLabel(QStringLiteral("耗时: -"), this);
    timeLabel->setObjectName(QStringLiteral("timeLabel"));

    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(companyBtn);
    statusBar()->addPermanentWidget(recordLabel);
    statusBar()->addPermanentWidget(timeLabel);
}

void MainWindow::setupToolbarIcons()
{
    // Helper: 根据绘制函数和颜色生成图标
    using IconPainter = std::function<void(QPainter&, const QRect&)>;
    auto makeIcon = [](const IconPainter& painter, int size, int pad, QColor color) -> QIcon {
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        QRect r(pad, pad, size - 2 * pad, size - 2 * pad);
        painter(p, r);
        p.end();
        return QIcon(pix);
    };

    // Helper: 为工具栏按钮设置浅色背景样式，让彩色图标可见
    auto styleToolbarBtn = [](QPushButton* btn, const QColor& accent) {
        btn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: #ffffff;
                color: #333333;
                border: 1px solid #d0d0d0;
                border-radius: 4px;
                padding: 4px 14px;
                font-size: 11px;
                min-height: 28px;
            }
            QPushButton:hover {
                background-color: %1;
                border-color: %2;
            }
            QPushButton:pressed {
                background-color: %3;
                border-color: %2;
            }
            QPushButton:disabled {
                background-color: #f5f5f5;
                color: #b0b0b0;
                border-color: #e0e0e0;
            }
        )").arg(accent.lighter(170).name(),
                accent.name(),
                accent.lighter(150).name()));
    };

    const int SIZE = 20;
    const int PAD = 3;
    const QSize ICON_SIZE(18, 18);

    // ========== 1. 导入Excel — 绿色 ==========
    QColor importColor("#27ae60");
    ui->importBtn->setIcon(makeIcon([](QPainter& p, const QRect& r) {
        int cx = r.center().x();
        p.drawLine(cx, r.top(), cx, r.bottom() - 4);
        p.drawLine(cx - 4, r.bottom() - 7, cx, r.bottom() - 3);
        p.drawLine(cx + 4, r.bottom() - 7, cx, r.bottom() - 3);
        p.drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
    }, SIZE, PAD, importColor));
    ui->importBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->importBtn, importColor);

    // ========== 1.5. 表头映射 — 青色 ==========
    QColor mappingColor("#00a8a8");
    ui->headerMappingBtn->setIcon(makeIcon([&](QPainter& p, const QRect& r) {
        int x1 = r.left() + 2;
        int x2 = r.center().x();
        int x3 = r.right() - 2;
        int y1 = r.top() + 2;
        int y2 = r.center().y();
        int y3 = r.bottom() - 2;
        // 左侧点阵
        p.setPen(QPen(mappingColor, 1.5));
        for (int i = 0; i < 3; i++) {
            int yy = y1 + i * 5;
            p.drawLine(x1, yy, x2 - 3, yy);
            p.setPen(QPen(mappingColor, 2.5));
            p.drawPoint(x1, yy);
            p.setPen(QPen(mappingColor, 1.5));
        }
        // 箭头
        p.setPen(QPen(mappingColor, 2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(x2, y2, x3, y2);
        p.drawLine(x3 - 5, y2 - 4, x3, y2);
        p.drawLine(x3 - 5, y2 + 4, x3, y2);
    }, SIZE, PAD, mappingColor));
    ui->headerMappingBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->headerMappingBtn, mappingColor);
    // 初始禁用，导入后启用
    ui->headerMappingBtn->setEnabled(false);

    // ========== 2. 开始计算 — 橙色 ==========
    QColor calcColor("#e67e22");
    ui->calculateBtn->setIcon(makeIcon([&](QPainter& p, const QRect& r) {
        int cx = r.center().x() + 1;
        int cy = r.center().y();
        p.setBrush(calcColor);
        QPolygon tri;
        tri << QPoint(cx - 4, cy - 6) << QPoint(cx + 5, cy) << QPoint(cx - 4, cy + 6);
        p.drawPolygon(tri);
    }, SIZE, PAD, calcColor));
    ui->calculateBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->calculateBtn, calcColor);

    // ========== 3. 导出结果 — 蓝色 ==========
    QColor exportColor("#2980b9");
    ui->exportBtn->setIcon(makeIcon([](QPainter& p, const QRect& r) {
        int cx = r.center().x();
        p.drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
        p.drawLine(cx, r.bottom() - 3, cx, r.top() + 3);
        p.drawLine(cx - 4, r.top() + 6, cx, r.top() + 2);
        p.drawLine(cx + 4, r.top() + 6, cx, r.top() + 2);
    }, SIZE, PAD, exportColor));
    ui->exportBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->exportBtn, exportColor);

    // ========== 4. 快速测试 — 琥珀色 ==========
    QColor testColor("#f39c12");
    ui->quickTestBtn->setIcon(makeIcon([&](QPainter& p, const QRect& r) {
        int cx = r.center().x();
        int cy = r.center().y();
        p.setBrush(testColor);
        QPolygon bolt;
        bolt << QPoint(cx, r.top()) << QPoint(cx - 4, cy + 1)
             << QPoint(cx + 1, cy) << QPoint(cx - 2, r.bottom())
             << QPoint(cx, cy - 1) << QPoint(cx - 3, cy)
             << QPoint(cx + 1, r.top());
        p.drawPolygon(bolt);
    }, SIZE, PAD, testColor));
    ui->quickTestBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->quickTestBtn, testColor);

    // ========== 5. 规则管理 — 紫色 ==========
    QColor ruleColor("#8e44ad");
    ui->ruleManageBtn->setIcon(makeIcon([&](QPainter& p, const QRect& r) {
        int y = r.top() + 3;
        for (int i = 0; i < 3; ++i) {
            int yy = y + i * 5;
            p.setPen(QPen(ruleColor, 1.5));
            p.drawLine(r.left() + 1, yy, r.right() - 1, yy);
            int knobX = r.left() + 3 + (i % 3) * 4;
            p.setPen(QPen(ruleColor, 2));
            p.drawPoint(knobX, yy);
        }
    }, SIZE, PAD, ruleColor));
    ui->ruleManageBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->ruleManageBtn, ruleColor);

    // ========== 6. 全局规则 — 青色 ==========
    QColor globalColor("#16a085");
    ui->globalRulesBtn->setIcon(makeIcon([](QPainter& p, const QRect& r) {
        p.drawEllipse(r.center(), 7, 7);
        p.drawLine(r.center().x(), r.top() + 2, r.center().x(), r.bottom() - 2);
        p.drawLine(r.left() + 2, r.center().y(), r.right() - 2, r.center().y());
    }, SIZE, PAD, globalColor));
    ui->globalRulesBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->globalRulesBtn, globalColor);

    // ========== 7. 规则说明 — 深蓝灰 ==========
    QColor helpColor("#2c3e50");
    ui->ruleHelpBtn->setIcon(makeIcon([&](QPainter& p, const QRect& r) {
        int cx = r.center().x();
        int cy = r.center().y();
        QRect arcRect(cx - 4, r.top(), 8, 8);
        p.drawArc(arcRect, 0 * 16, 180 * 16);
        p.drawLine(cx, cy - 1, cx, cy + 2);
        p.setBrush(helpColor);
        p.drawEllipse(QPoint(cx, r.bottom()), 1, 1);
    }, SIZE, PAD, helpColor));
    ui->ruleHelpBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->ruleHelpBtn, helpColor);

    // ========== 8. 历史记录 — 深橙 ==========
    QColor historyColor("#d35400");
    ui->historyBtn->setIcon(makeIcon([](QPainter& p, const QRect& r) {
        int cx = r.center().x();
        int cy = r.center().y();
        p.drawEllipse(r.center(), 7, 7);
        p.drawLine(cx, cy, cx, cy - 4);
        p.drawLine(cx, cy, cx + 4, cy);
    }, SIZE, PAD, historyColor));
    ui->historyBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->historyBtn, historyColor);

    // ========== 9. 生成图表 — 红色 ==========
    QColor chartColor("#e74c3c");
    ui->chartBtn->setIcon(makeIcon([&](QPainter& p, const QRect& r) {
        int barW = 4;
        int gap = 1;
        int baseY = r.bottom();
        int heights[] = {6, 10, 7};
        for (int i = 0; i < 3; ++i) {
            int bx = r.left() + 1 + i * (barW + gap);
            QRect bar(bx, baseY - heights[i], barW, heights[i]);
            p.setBrush(chartColor);
            p.setPen(Qt::NoPen);
            p.drawRect(bar);
        }
    }, SIZE, PAD, chartColor));
    ui->chartBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->chartBtn, chartColor);

    // ========== 10. clearBtn ==========
    QColor clearColor("#95a5a6");
    ui->clearBtn->setIcon(makeIcon([](QPainter& p, const QRect& r) {
        int cx = r.center().x(), cy = r.center().y();
        p.drawLine(r.left()+3, r.top()+3, r.right()-3, r.bottom()-3);
        p.drawLine(r.right()-3, r.top()+3, r.left()+3, r.bottom()-3);
    }, SIZE, PAD, clearColor));
    ui->clearBtn->setIconSize(ICON_SIZE);
    styleToolbarBtn(ui->clearBtn, clearColor);
}

void MainWindow::showCenterProgress(const QString &text)
{
    m_centerProgressLabel->setText(text);
    m_centerProgress->setValue(0);
    m_lastProgressPercent = -1;
    m_lastProgressTime.start();

    QWidget *parent = ui->dataTableView->parentWidget();
    QPoint topLeft = ui->dataTableView->mapTo(parent, QPoint(0, 0));
    m_overlayWidget->setGeometry(
        topLeft.x(), topLeft.y(),
        ui->dataTableView->width(),
        ui->dataTableView->height());
    m_overlayWidget->raise();

    // 如果已可见，直接刷新内容；否则先显示再刷新
    if (!m_overlayWidget->isVisible()) {
        m_overlayWidget->show();
    }
    m_overlayWidget->repaint();
}

void MainWindow::hideCenterProgress()
{
    m_overlayWidget->hide();
}

void MainWindow::updateCenterProgress(int percent)
{
    if (m_centerProgress && m_overlayWidget) {
        m_centerProgress->setValue(percent);
        if (m_calcTotalRows > 0 && percent >= 0 && percent < 100) {
            int done = static_cast<int>(percent / 100.0 * m_calcTotalRows);
            m_centerProgressLabel->setText(QStringLiteral("正在计算运费... %1 / %2 条").arg(done).arg(m_calcTotalRows));
        }
    }
}

void MainWindow::throttledProgress(int percent)
{
    if (percent == m_lastProgressPercent)
        return;

    if (percent == 0 || percent == 100 || percent - m_lastProgressPercent >= 1) {
        if (percent != 100 && percent != 0 && m_lastProgressTime.isValid() && m_lastProgressTime.elapsed() < 30) {
            return;
        }
        m_lastProgressPercent = percent;
        m_lastProgressTime.restart();
        updateProgress(percent);
    }
}

void MainWindow::applyLightStyle()
{
    QString style = R"(
        QMainWindow {
            background-color: #f5f6fa;
        }

        QPushButton {
            background-color: #4a90d9;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 4px 14px;
            font-size: 11px;
            min-height: 28px;
        }
        QPushButton:hover {
            background-color: #3a7bc8;
        }
        QPushButton:pressed {
            background-color: #2d6bb3;
        }
        QPushButton:disabled {
            background-color: #d0d0d0;
            color: #808080;
        }

        QComboBox {
            background-color: white;
            color: #333333;
            border: 1px solid #d0d0d0;
            border-radius: 4px;
            padding: 3px 8px;
            font-size: 11px;
            min-height: 24px;
        }
        QComboBox:hover {
            border-color: #4a90d9;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid #666666;
            margin-right: 4px;
        }
        QComboBox QAbstractItemView {
            background-color: white;
            color: #333333;
            selection-background-color: #e0e8f5;
            selection-color: #2d6bb3;
            border: 1px solid #d0d0d0;
            outline: none;
            font-size: 11px;
        }

        QSpinBox {
            background-color: white;
            color: #333333;
            border: 1px solid #d0d0d0;
            border-radius: 4px;
            padding: 3px 8px;
            font-size: 11px;
            min-height: 24px;
        }
        QSpinBox:hover {
            border-color: #4a90d9;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background-color: #f0f0f0;
            border: none;
            width: 18px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #e0e0e0;
        }
        QSpinBox::up-arrow {
            border-left: 3px solid transparent;
            border-right: 3px solid transparent;
            border-bottom: 4px solid #666666;
        }
        QSpinBox::down-arrow {
            border-left: 3px solid transparent;
            border-right: 3px solid transparent;
            border-top: 4px solid #666666;
        }

        QTableView {
            background-color: white;
            alternate-background-color: #fafafa;
            color: #333333;
            gridline-color: #e0e0e0;
            border: 1px solid #d0d0d0;
            border-radius: 4px;
            font-size: 11px;
            selection-background-color: #e0e8f5;
            selection-color: #2d6bb3;
            outline: none;
        }
        QTableView::item {
            padding: 3px 6px;
        }
        QHeaderView::section {
            background-color: #f5f6fa;
            color: #555555;
            padding: 4px 6px;
            border: none;
            border-right: 1px solid #e0e0e0;
            border-bottom: 1px solid #e0e0e0;
            font-weight: bold;
            font-size: 11px;
        }
        QHeaderView::section:hover {
            background-color: #e8eaef;
        }

        QProgressBar {
            background-color: #e0e0e0;
            border: 1px solid #d0d0d0;
            border-radius: 4px;
            text-align: center;
            color: #555555;
            font-size: 10px;
            min-height: 16px;
        }
        QProgressBar::chunk {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #4a90d9, stop:1 #3a7bc8);
            border-radius: 3px;
        }

        QGroupBox {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 12px;
            color: #333333;
            font-size: 12px;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 2px 8px;
            color: #4a90d9;
        }

        QLabel {
            color: #555555;
            font-size: 11px;
        }

        QStatusBar {
            background-color: #f5f6fa;
            color: #888888;
            font-size: 10px;
            border-top: 1px solid #e0e0e0;
        }
        QStatusBar::item {
            border: none;
            padding: 0 6px;
        }
        QStatusBar QLabel {
            color: #888888;
            font-size: 10px;
            padding: 0 4px;
        }

        QSplitter::handle {
            background-color: #d0d0d0;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }
        QSplitter::handle:hover {
            background-color: #4a90d9;
        }

        QScrollBar:vertical {
            background-color: #f0f0f0;
            width: 8px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background-color: #c0c0c0;
            border-radius: 4px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #4a90d9;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background-color: #f0f0f0;
            height: 8px;
            border: none;
        }
        QScrollBar::handle:horizontal {
            background-color: #c0c0c0;
            border-radius: 4px;
            min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #4a90d9;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
    )";

    qApp->setStyleSheet(style);
}

void MainWindow::onImportClicked()
{
    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("正在打开文件选择对话框..."));

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("选择Excel文件（最多5个）"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        QStringLiteral("Excel文件 (*.xlsx *.xls)"),
        nullptr,
        QFileDialog::DontUseNativeDialog
    );

    if (statusLabel)
        statusLabel->setText(QStringLiteral("文件选择完成"));

    if (filePaths.isEmpty())
        return;

    if (filePaths.size() > 5) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("最多选择5个文件进行处理"));
        filePaths = filePaths.mid(0, 5);
    }

    m_currentFilePaths = filePaths;
    m_currentOrders.clear();
    m_currentPage = 1;
    m_lastDedupCount = 0;

    // ★ 预检测关键字段，不匹配则先弹表头映射再导入
    {
        ExcelImporter detector;
        QStringList headers = detector.getAvailableHeaders(filePaths.first());
        QMap<QString, int> mapping = detector.autoDetectColumns(headers);

        m_lastImportedFilePath = filePaths.first();
        m_lastImportedHeaders = headers;
        m_lastColumnMapping = mapping;

                QStringList required = {QStringLiteral("运单号"), QStringLiteral("业务时间"), QStringLiteral("客户"), QStringLiteral("目的省份"), QStringLiteral("实际重量")};
        QStringList missing;
        QMap<QString, QString> labels = {
            {QStringLiteral("客户"),     QStringLiteral("结算对象(客户)")},
            {QStringLiteral("目的省份"), QStringLiteral("目的省份(运单送达地)")},
            {QStringLiteral("实际重量"), QStringLiteral("结算重量")}
        };
        for (const QString &f : required) {
            if (!mapping.contains(f) || mapping[f] < 0)
                missing.append(f);
        }
        if (!missing.isEmpty()) {
            QString names;
            for (const QString &f : missing)
                names += QStringLiteral("「%1」").arg(labels.value(f, f));
            QMessageBox::information(this, QStringLiteral("关键字段未匹配"),
                QStringLiteral("以下关键字段未能自动匹配：\n%1\n\n请在弹出的表头映射对话框中手动指定。").arg(names));

            m_lastImportedHeaders = headers;
            m_lastColumnMapping = mapping;
            m_lastImportedFilePath = filePaths.first();
            onHeaderMappingClicked();
            return;
        }
    }

    if (statusLabel)
        statusLabel->setText(QStringLiteral("正在导入..."));

    ui->progressBar->setValue(0);
    ui->importBtn->setEnabled(false);
    ui->calculateBtn->setEnabled(false);
    ui->exportBtn->setEnabled(false);

    showCenterProgress(QStringLiteral("正在导入文件..."));
    // ★ 列选择对话框
    {
        QStringList requiredKeys = {QStringLiteral("运单号"), QStringLiteral("业务时间"), QStringLiteral("订单客户"),
                                     QStringLiteral("客户"), QStringLiteral("目的省份"), QStringLiteral("实际重量")};

        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("选择导入列"));
        dlg.setMinimumSize(420, 480);
        auto *dlgLayout = new QVBoxLayout(&dlg);
        dlgLayout->addWidget(new QLabel(QStringLiteral("勾选需要导入的列（灰色=必选，不可取消）："), &dlg));
        auto *list = new QListWidget(&dlg);
        for (const QString &h : m_lastImportedHeaders) {
            auto *item = new QListWidgetItem(h, list);
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            bool isRequired = false;
            for (const QString &rk : requiredKeys) {
                if (m_lastColumnMapping.contains(rk) && m_lastColumnMapping[rk] >= 0
                    && m_lastColumnMapping[rk] < m_lastImportedHeaders.size()
                    && m_lastImportedHeaders[m_lastColumnMapping[rk]] == h) {
                    isRequired = true; break;
                }
            }
            if (isRequired) {
                item->setCheckState(Qt::Checked);
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            } else {
                bool matched = false;
                for (auto it = m_lastColumnMapping.constBegin(); it != m_lastColumnMapping.constEnd(); ++it) {
                    if (it.value() >= 0 && it.value() < m_lastImportedHeaders.size()
                        && m_lastImportedHeaders[it.value()] == h) {
                        matched = true; break;
                    }
                }
                item->setCheckState(matched ? Qt::Checked : Qt::Unchecked);
            }
        }
        dlgLayout->addWidget(list);
        auto *btnLayout = new QHBoxLayout();
        auto *allBtn = new QPushButton(QStringLiteral("全选"), &dlg);
        auto *noneBtn = new QPushButton(QStringLiteral("全不选"), &dlg);
        auto *okBtn = new QPushButton(QStringLiteral("确定"), &dlg);
        okBtn->setObjectName(QStringLiteral("primaryButton"));
        btnLayout->addWidget(allBtn); btnLayout->addWidget(noneBtn);
        btnLayout->addStretch(); btnLayout->addWidget(okBtn);
        dlgLayout->addLayout(btnLayout);

        QObject::connect(allBtn, &QPushButton::clicked, [&]() {
            for (int i = 0; i < list->count(); ++i) {
                if (list->item(i)->flags() & Qt::ItemIsEnabled)
                    list->item(i)->setCheckState(Qt::Checked);
            }
        });
        QObject::connect(noneBtn, &QPushButton::clicked, [&]() {
            for (int i = 0; i < list->count(); ++i) {
                if (list->item(i)->flags() & Qt::ItemIsEnabled)
                    list->item(i)->setCheckState(Qt::Unchecked);
            }
        });
        QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

        if (dlg.exec() != QDialog::Accepted)
            return;

        for (int i = 0; i < list->count(); ++i) {
            if (list->item(i)->checkState() != Qt::Checked) {
                QString header = list->item(i)->text();
                for (auto it = m_lastColumnMapping.begin(); it != m_lastColumnMapping.end(); ) {
                    if (it.key().startsWith(QStringLiteral("__"))) { ++it; continue; }  // skip marker keys
                    int col = it.value();
                    if (col >= 0 && col < m_lastImportedHeaders.size() && m_lastImportedHeaders[col] == header)
                        it = m_lastColumnMapping.erase(it);
                    else
                        ++it;
                }
            }
        }
    }

    emit importProgress(0);

    importFilesAsync(filePaths, false, ui->sheet1OnlyCheck->isChecked());
}

void MainWindow::importFilesAsync(const QStringList &filePaths, bool useExistingMapping, bool sheet1Only)
{
    m_totalFileCount = filePaths.size();
    m_processedFileCount = 0;

    // ★ 预检测第一个文件的表头和列映射
    // useExistingMapping=true → 使用已有的 m_lastColumnMapping（表头映射对话框设置的值）
    // useExistingMapping=false → 自动检测（首次导入）
    if (!useExistingMapping) {
        m_lastImportedFilePath = filePaths.first();
        ExcelImporter detector;
        m_lastImportedHeaders = detector.getAvailableHeaders(m_lastImportedFilePath);
        m_lastColumnMapping = detector.autoDetectColumns(m_lastImportedHeaders);
    }

    QPointer<MainWindow> self = this;

    auto sharedOrders = std::make_shared<QList<OrderData>>();
    auto sharedMutex = std::make_shared<QMutex>();
    auto sharedProcessedRows = std::make_shared<QAtomicInt>(0);
    auto sharedTotalRows = std::make_shared<QAtomicInt>(0);
    auto sharedMaxProgress = std::make_shared<QAtomicInt>(0);

    QMap<QString, int> columnMapping = m_lastColumnMapping;
    auto sharedSheet1Only = std::make_shared<bool>(sheet1Only);

    emit importProgress(0);

    auto processFunc = [self, filePaths, sharedOrders, sharedMutex, sharedProcessedRows,
                        sharedTotalRows, sharedMaxProgress, sharedSheet1Only, columnMapping]() {
        try {
            int fileCount = filePaths.size();

            // ★ 不再单独 scan 行数（浪费一遍完整解析）——用文件大小估算
            int total = 0;
            for (int i = 0; i < fileCount; ++i) {
                QFileInfo fi(filePaths[i]);
                // 经验公式: 1MB ~ 5000 行（xlsx 压缩比约 5:1，每行约 200 字节压缩后）
                int estRows = static_cast<int>(fi.size() / 200);
                total += estRows;
            }
            sharedTotalRows->storeRelease(total);

            if (total > 0) {
                sharedOrders->reserve(total + 1000);

                // 按文件并行导入：每文件独立线程，各自 ZipReader 互不干扰
                QList<QFuture<void>> importFutures;
                for (int i = 0; i < fileCount; ++i) {
                    importFutures.append(QtConcurrent::run([&, i]() {
                        ExcelImporter importer;
                        QMap<QString, int> cm = columnMapping;
                        if (*sharedSheet1Only) cm[QStringLiteral("__sheet1_only__")] = 1;

                        // ★ 文件内部进度信号 → 实时更新进度条
                        QObject::connect(&importer, &ExcelImporter::progressUpdated, self.data(),
                            [&](int filePct) {
                                int localDone = static_cast<int>(filePct / 100.0 * static_cast<int>(QFileInfo(filePaths[i]).size() / 200));
                                int globalDone = sharedProcessedRows->loadAcquire() + localDone;
                                int tot = sharedTotalRows->loadAcquire();
                                if (tot > 0) {
                                    int pct = static_cast<int>(globalDone * 100.0 / tot);
                                    int prev = sharedMaxProgress->loadAcquire();
                                    while (pct > prev && !sharedMaxProgress->testAndSetOrdered(prev, pct))
                                        prev = sharedMaxProgress->loadAcquire();
                                    if (pct > prev && self) self->importProgress(pct);
                                }
                            }, Qt::QueuedConnection);

                        QList<OrderData> orders = importer.importFromFile(filePaths[i], cm);

                        QMutexLocker locker(sharedMutex.get());
                        sharedOrders->append(orders);
                        sharedProcessedRows->fetchAndAddOrdered(orders.size());

                        int done = sharedProcessedRows->loadAcquire();
                        int tot = sharedTotalRows->loadAcquire();
                        if (tot > 0) {
                            int pct = static_cast<int>(done * 100.0 / tot);
                            int prev = sharedMaxProgress->loadAcquire();
                            while (pct > prev && !sharedMaxProgress->testAndSetOrdered(prev, pct))
                                prev = sharedMaxProgress->loadAcquire();
                            if (pct > prev && self) self->importProgress(pct);
                        }
                    }));
                }
                for (auto &f : importFutures) f.waitForFinished();

                // ★ O(n) 去重：新建列表过滤，避免 erase() 的 O(n²) 移动
                if (!sharedOrders->isEmpty()) {
                    int before = sharedOrders->size();
                    QSet<QString> seen;
                    seen.reserve(before);
                    QList<OrderData> unique;
                    unique.reserve(before);
                    for (const OrderData &order : *sharedOrders) {
                        if (!order.waybillNo.isEmpty() && !seen.contains(order.waybillNo)) {
                            seen.insert(order.waybillNo);
                            unique.append(order);
                        }
                    }
                    int after = unique.size();
                    *sharedOrders = std::move(unique);
                    sharedTotalRows->storeRelease(after);
                    int dupCount = before - after;
                    QMetaObject::invokeMethod(self.data(), [self, dupCount]() {
                        if (self) self->m_lastDedupCount = dupCount;
                    }, Qt::QueuedConnection);
                }
            }
        } catch (const std::exception &e) {
            QString errorMsg = QStringLiteral("导入异常: %1").arg(QString::fromUtf8(e.what()));
            Logger::instance().error(errorMsg);
            QMetaObject::invokeMethod(self.data(), [self, errorMsg]() {
                if (self) {
                    auto *statusLabel = self->statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
                    if (statusLabel)
                        statusLabel->setText(errorMsg);
                    QMessageBox::warning(self, QStringLiteral("导入错误"), errorMsg);
                }
            }, Qt::QueuedConnection);
        } catch (...) {
            QString errorMsg = QStringLiteral("导入发生未知错误");
            Logger::instance().error(errorMsg);
            QMetaObject::invokeMethod(self.data(), [self, errorMsg]() {
                if (self) {
                    auto *statusLabel = self->statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
                    if (statusLabel)
                        statusLabel->setText(errorMsg);
                    QMessageBox::warning(self, QStringLiteral("导入错误"), errorMsg);
                }
            }, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(self.data(), [self, sharedOrders]() {
            if (self) {
                self->m_currentOrders = std::move(*sharedOrders);
                emit self->importProgress(100);
                self->onImportFinished();
            }
        }, Qt::QueuedConnection);
    };

    QFuture<void> future = QtConcurrent::run(processFunc);
    m_importWatcher->setFuture(future);
}

void MainWindow::onImportFinished()
{
    bool willAutoCalc = !m_currentOrders.isEmpty();

    // ★ 如果即将自动计算，不隐藏 overlay，让计算直接复用并更新内容
    if (!willAutoCalc) {
        hideCenterProgress();
    }
    ui->importBtn->setEnabled(true);
    ui->headerMappingBtn->setEnabled(true);
    ui->calculateBtn->setEnabled(true);
    ui->exportBtn->setEnabled(true);

    if (willAutoCalc) {
        m_currentPage = 1;
        updateTableView();

        auto *recordLabel = statusBar()->findChild<QLabel*>(QStringLiteral("recordLabel"));
        if (recordLabel)
            recordLabel->setText(QStringLiteral("记录数: %1").arg(m_currentOrders.size()));
    }

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel) {
        if (m_lastDedupCount > 0)
            statusLabel->setText(QStringLiteral("导入完成，共 %1 条（去重 %2 条）").arg(m_currentOrders.size()).arg(m_lastDedupCount));
        else
            statusLabel->setText(QStringLiteral("导入完成，共 %1 条").arg(m_currentOrders.size()));
    }

    ui->progressBar->setValue(100);

    // 导入完成后自动开始计算
    if (willAutoCalc) {
        onCalculateClicked();
    }
}

void MainWindow::onCalculateClicked()
{
    if (m_currentOrders.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请先导入订单数据。"));
        return;
    }

    if (m_isCalculating) {
        return;
    }
    m_isCalculating = true;

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("正在计算..."));

    ui->startCalcBtn->setEnabled(false);
    ui->calculateBtn->setEnabled(false);
    ui->importBtn->setEnabled(false);
    ui->headerMappingBtn->setEnabled(false);
    ui->progressBar->setValue(0);

    showCenterProgress(QStringLiteral("正在计算运费..."));
    m_calcTotalRows = m_currentOrders.size();  // ★ 必须在计算前设置总行数
    emit calcProgress(0);
    QApplication::processEvents();

    // 保存当前计算上下文，用于历史记录
    m_currentCalcMode = ui->modeCombo->currentText();
    m_currentCustomer = ui->customerCombo->currentText();
    int rawThreadCount = ui->threadSpin->value();
    // 0 = 自动，使用 CPU 逻辑核心数
    int effectiveThreadCount = (rawThreadCount == 0) ? QThread::idealThreadCount() : rawThreadCount;
    m_currentThreadCount = effectiveThreadCount;
    m_currentAvgBase = ui->avgBaseSpin->value();
    m_currentAvgLimit = ui->avgLimitSpin->value();
    m_currentAvgStep = ui->avgStepSpin->value();
    m_currentAvgSurcharge = ui->avgSurchargeSpin->value();

    QPointer<MainWindow> self = this;
    QString mode = ui->modeCombo->currentText();
    int threadCount = effectiveThreadCount;
    QString selectedCustomer = ui->customerCombo->currentText();
    QMap<QString, CustomerRule> ruleMap;
    QStringList ruleNames = m_ruleManager->ruleNames();
    for (const QString &name : ruleNames) {
        ruleMap[name] = m_ruleManager->getRule(name);
    }
    QString firstRuleName = ruleNames.isEmpty() ? QString() : ruleNames.first();
    QList<PriceRule> defaultPriceTable = m_calculator->defaultPriceTable();
    GlobalRules globalRules = m_calculator->globalRules();

    // 读取拉均重参数
    globalRules.avgWeightBase = ui->avgBaseSpin->value();
    globalRules.avgWeightUpperLimit = ui->avgLimitSpin->value();
    globalRules.avgWeightIncrement = ui->avgStepSpin->value();
    globalRules.avgWeightSurcharge = ui->avgSurchargeSpin->value();

    QList<OrderData> orders = m_currentOrders;

    // 根据控制面板的客户选择过滤数据
    if (!selectedCustomer.isEmpty() && selectedCustomer != QStringLiteral("全部")) {
        QList<OrderData> filtered;
        filtered.reserve(orders.size());
        for (const OrderData &order : orders) {
            QString client = order.client.isEmpty() ? order.customer : order.client;
            if (client == selectedCustomer) {
                filtered.append(order);
            }
        }
        orders = std::move(filtered);
    }

    // Province normalization: standardize all destination provinces for O(1) lookup
    for (OrderData &o : orders)
        o.destinationProvince = ProvinceUtils::standardize(o.destinationProvince);

        auto calcFunc = [self, orders = std::move(orders), mode, threadCount, ruleMap, firstRuleName,
                     defaultPriceTable, globalRules]() mutable {
        if (!self) return;

        int totalCount = orders.size();
        QAtomicInt processed(0);
        QAtomicInt errorCount(0);
        QAtomicInt lastPercent(-1);  // ★ 防重复上报，替代 thread_local
        int effectiveThreads = qMax(1, qMin(threadCount, QThread::idealThreadCount()));

        QMap<QString, QPair<int, int>> clientRanges;
        QMap<QString, int> clientCounts;

        for (int i = 0; i < totalCount; ++i) {
            const OrderData &order = orders[i];
            QString client = order.client.isEmpty() ? order.customer : order.client;
            clientCounts[client]++;
        }

        int pos = 0;
        for (auto it = clientCounts.constBegin(); it != clientCounts.constEnd(); ++it) {
            clientRanges[it.key()] = qMakePair(pos, pos + it.value());
            pos += it.value();
        }

        QList<OrderData> sortedOrders;
        sortedOrders.reserve(totalCount);
        sortedOrders.resize(totalCount);

        QMap<QString, int> clientInsertPos;
        for (auto it = clientRanges.constBegin(); it != clientRanges.constEnd(); ++it) {
            clientInsertPos[it.key()] = it.value().first;
        }

        for (int i = 0; i < totalCount; ++i) {
            const OrderData &order = orders[i];
            QString client = order.client.isEmpty() ? order.customer : order.client;
            int &insertIdx = clientInsertPos[client];
            sortedOrders[insertIdx] = order;
            insertIdx++;
        }

        orders.clear();

        QList<QFuture<void>> calcFutures;

        for (auto it = clientRanges.constBegin(); it != clientRanges.constEnd(); ++it) {
            QString client = it.key();
            int start = it.value().first;
            int end = it.value().second;
            int count = end - start;
            if (count <= 0) continue;

            CustomerRule rule = ruleMap.value(client);
            if (rule.customerName.isEmpty()) {
                if (!firstRuleName.isEmpty()) {
                    rule = ruleMap.value(firstRuleName);
                } else {
                    rule.customerName = client;
                    rule.useDefaultPrice = true;
                }
            }
            rule.calculationMode = mode;

            // 拉均重模式：计算客户平均重量和每包裹加价
            GlobalRules clientGlobalRules = globalRules;
            if (mode == QStringLiteral("拉均重")) {
                double totalWeight = 0.0;
                int validCount = 0;
                for (int i = start; i < end; ++i) {
                    double w = std::max(sortedOrders[i].actualWeight, sortedOrders[i].volumetricWeight);
                    if (w > 0) {
                        totalWeight += w;
                        validCount++;
                    }
                }
                if (validCount > 0) {
                    double avgWeight = totalWeight / validCount;
                    double base = globalRules.avgWeightBase;
                    double limit = globalRules.avgWeightUpperLimit;
                    double step = globalRules.avgWeightIncrement;
                    double surcharge = globalRules.avgWeightSurcharge;

                    if (avgWeight > base && step > 0) {
                        // 超出部分封顶在上限
                        double exceeded = std::min(avgWeight, limit) - base;
                        if (exceeded > 0) {
                            int increments = static_cast<int>(std::ceil(exceeded / step));
                            clientGlobalRules.runtimeAvgSurcharge = increments * surcharge;
                        }
                    }
                }
            }

            int chunks = qMax(1, qMin(effectiveThreads, count / 5000 + 1));
            int chunkSize = (count + chunks - 1) / chunks;

            for (int c = 0; c < chunks; ++c) {
                int cs = start + c * chunkSize;
                int ce = qMin(start + (c + 1) * chunkSize, end);
                if (cs >= ce) continue;

                calcFutures.append(QtConcurrent::run([&, cs, ce, rule, clientGlobalRules]() {
                    OrderData *dataPtr = sortedOrders.data();
                    int chunkCount = ce - cs;

                    // SIMD加速批量计算
                    int errors = SimdCalculator::calculateChunk(
                        dataPtr + cs, chunkCount, rule, defaultPriceTable, clientGlobalRules
                    );
                    errorCount.fetchAndAddRelaxed(errors);

                    int done = processed.fetchAndAddRelaxed(chunkCount) + chunkCount;
                    int percent = static_cast<int>(done * 100.0 / totalCount);
                    int prevPct = lastPercent.loadAcquire();
                    while (percent > prevPct && !lastPercent.testAndSetOrdered(prevPct, percent))
                        prevPct = lastPercent.loadAcquire();
                    if (percent > prevPct || done == totalCount) {
                        if (self) {
                            self->calcProgress(qMin(99, percent));
                        }
                    }
                }));
            }
        }

        for (auto &f : calcFutures)
            f.waitForFinished();

        QMetaObject::invokeMethod(self.data(), [self, sortedOrders = std::move(sortedOrders), totalCount, errorCount]() mutable {
            if (!self) return;
            self->m_currentOrders = std::move(sortedOrders);
            self->onCalculationComplete(totalCount, errorCount.loadAcquire());
        }, Qt::QueuedConnection);
    };

    QFuture<void> future = QtConcurrent::run(calcFunc);
    m_batchWatcher->setFuture(future);
}

void MainWindow::onExportClicked()
{
    if (m_currentOrders.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("没有可导出的数据。"));
        return;
    }

    QDialog exportDialog(this);
    exportDialog.setWindowTitle(QStringLiteral("导出选项"));
    exportDialog.setMinimumWidth(300);

    auto *layout = new QVBoxLayout(&exportDialog);

    auto *mergeCheck = new QCheckBox(QStringLiteral("合并导出（所有数据导出到一个文件）"), &exportDialog);
    mergeCheck->setChecked(true);

    auto *perClientCheck = new QCheckBox(QStringLiteral("按客户分别导出（每个客户一个文件）"), &exportDialog);
    perClientCheck->setChecked(false);

    auto *summaryCheck = new QCheckBox(QStringLiteral("导出汇总表"), &exportDialog);
    summaryCheck->setChecked(true);

    layout->addWidget(mergeCheck);
    layout->addWidget(perClientCheck);
    layout->addWidget(summaryCheck);

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &exportDialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &exportDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &exportDialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (exportDialog.exec() != QDialog::Accepted)
        return;

    bool mergeExport = mergeCheck->isChecked();
    bool perClientExport = perClientCheck->isChecked();
    bool exportSummary = summaryCheck->isChecked();

    if (!mergeExport && !perClientExport) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请至少选择一种导出方式。"));
        return;
    }

    QString outputDir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择导出目录"),
        QString(),
        QFileDialog::DontUseNativeDialog
    );

    if (outputDir.isEmpty())
        return;

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("正在导出..."));

    ui->progressBar->setValue(0);
    ui->exportBtn->setEnabled(false);

    showCenterProgress(QStringLiteral("正在导出数据..."));

    exportFilesAsync(outputDir, mergeExport, perClientExport, exportSummary);
}

void MainWindow::exportFilesAsync(const QString &outputDir, bool mergeExport, bool perClientExport, bool exportSummary)
{
    QPointer<MainWindow> self = this;
    QList<OrderData> orders = m_currentOrders;

    auto exportFunc = [self, outputDir, orders, mergeExport, perClientExport, exportSummary]() {
        if (!self) return;

        try {
            QDir dir(outputDir);
            int totalSteps = (mergeExport ? 1 : 0) + (perClientExport ? 1 : 0);
            int step = 0;

            if (mergeExport) {
                QString fileName = QStringLiteral("运费计算结果_全部_%1.xlsx")
                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
                QString filePath = dir.filePath(fileName);

                ExcelExporter exporter;
                QObject::connect(&exporter, &ExcelExporter::progressUpdated, self.data(),
                    [self, step, totalSteps](int filePercent) {
                        if (!self) return;
                        int overall = static_cast<int>((step * 100.0 + filePercent) / totalSteps);
                        self->throttledProgress(overall);
                        self->updateCenterProgress(overall);
                    }, Qt::QueuedConnection);

                exporter.exportToFile(filePath, orders);
                step++;

                if (exportSummary) {
                    QString summaryName = QStringLiteral("运费汇总_%1.xlsx")
                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
                    exporter.exportSummary(dir.filePath(summaryName), orders);
                }
            }

            if (perClientExport) {
                QMap<QString, QList<OrderData>> ordersByClient;
                for (const OrderData &order : orders) {
                    QString client = order.client.isEmpty() ? order.customer : order.client;
                    if (client.isEmpty())
                        client = QStringLiteral("未知客户");
                    ordersByClient[client].append(order);
                }

                int clientIndex = 0;
                int totalClients = ordersByClient.size();

                for (auto it = ordersByClient.constBegin(); it != ordersByClient.constEnd(); ++it) {
                    QString client = it.key();
                    QString safeName = client;
                    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

                    QString fileName = QStringLiteral("运费结果_%1_%2.xlsx")
                        .arg(safeName)
                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
                    QString filePath = dir.filePath(fileName);

                    ExcelExporter exporter;
                    exporter.exportToFile(filePath, it.value());

                    clientIndex++;
                    int overall = static_cast<int>(((step + clientIndex * 1.0 / totalClients) * 100.0) / (step + 1));
                    QMetaObject::invokeMethod(self.data(), [self, overall]() {
                        if (self) {
                            self->throttledProgress(overall);
                            self->updateCenterProgress(overall);
                        }
                    }, Qt::QueuedConnection);
                }
            }
        } catch (...) {
            QMetaObject::invokeMethod(self.data(), [self]() {
                if (self) {
                    auto *statusLabel = self->statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
                    if (statusLabel)
                        statusLabel->setText(QStringLiteral("导出发生错误"));
                }
            }, Qt::QueuedConnection);
        }
    };

    QFuture<void> future = QtConcurrent::run(exportFunc);
    m_exportWatcher->setFuture(future);
}

void MainWindow::onQuickTestClicked()
{
    QuickTestWidget dialog(m_ruleManager, this);
    dialog.exec();
}

void MainWindow::onRuleManageClicked()
{
    RuleManagerDialog dialog(m_ruleManager, this);
    dialog.exec();
    m_calculator->setDefaultPriceTable(m_ruleManager->defaultPriceTable());
    updateCustomerCombo();
}

void MainWindow::onGlobalRulesClicked()
{
    GlobalRulesDialog dialog(m_ruleManager, this);
    dialog.exec();
    GlobalRules gr = m_ruleManager->globalRules();
    m_calculator->setGlobalRules(gr);

    // 同步全局规则中的拉均重参数到主界面（阻塞信号避免循环触发保存）
    ui->avgBaseSpin->blockSignals(true);
    ui->avgLimitSpin->blockSignals(true);
    ui->avgStepSpin->blockSignals(true);
    ui->avgSurchargeSpin->blockSignals(true);
    ui->avgBaseSpin->setValue(gr.avgWeightBase);
    ui->avgLimitSpin->setValue(gr.avgWeightUpperLimit);
    ui->avgStepSpin->setValue(gr.avgWeightIncrement);
    ui->avgSurchargeSpin->setValue(gr.avgWeightSurcharge);
    ui->avgBaseSpin->blockSignals(false);
    ui->avgLimitSpin->blockSignals(false);
    ui->avgStepSpin->blockSignals(false);
    ui->avgSurchargeSpin->blockSignals(false);
}

void MainWindow::onCalcModeChanged(const QString &mode)
{
    bool showAvg = (mode == QStringLiteral("拉均重"));

    QFormLayout *formLayout = ui->formLayout;
    if (!formLayout)
        return;

    // 遍历 formLayout 中的所有行，找到拉均重相关的行并设置可见性
    // 通过控件指针查找行号，确保即使UI行顺序变化也能正确匹配
    QWidget *avgWidgets[] = {
        ui->avgBaseSpin, ui->avgLimitSpin, ui->avgStepSpin, ui->avgSurchargeSpin
    };

    for (QWidget *widget : avgWidgets) {
        int row = -1;
        QFormLayout::ItemRole role;
        int index = formLayout->indexOf(widget);
        if (index >= 0) {
            formLayout->getItemPosition(index, &row, &role);
            if (row >= 0) {
                formLayout->setRowVisible(row, showAvg);
            }
        }
    }
}

void MainWindow::onAvgParamChanged()
{
    GlobalRules rules = m_ruleManager->globalRules();
    rules.avgWeightBase = ui->avgBaseSpin->value();
    rules.avgWeightUpperLimit = ui->avgLimitSpin->value();
    rules.avgWeightIncrement = ui->avgStepSpin->value();
    rules.avgWeightSurcharge = ui->avgSurchargeSpin->value();
    m_ruleManager->setGlobalRules(rules);
    m_calculator->setGlobalRules(rules);
    m_ruleManager->saveToFile(RuleManager::defaultFilePath());
}

void MainWindow::updateProgress(int percent)
{
    ui->progressBar->setValue(percent);

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("进度: %1%").arg(percent));
}

void MainWindow::onCalculationComplete(int totalCount, int errorCount)
{
    m_isCalculating = false;
    hideCenterProgress();
    ui->startCalcBtn->setEnabled(true);
    ui->calculateBtn->setEnabled(true);
    ui->importBtn->setEnabled(true);
    ui->headerMappingBtn->setEnabled(true);
    ui->progressBar->setValue(100);

    m_currentPage = 1;
    updateTableView();

    // 保存计算历史记录
    saveCalculationHistory();

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("计算完成"));

    QMessageBox::information(this, QStringLiteral("计算完成"),
                             QStringLiteral("总计: %1 条\n成功: %2 条\n失败: %3 条")
                             .arg(totalCount)
                             .arg(totalCount - errorCount)
                             .arg(errorCount));
}

void MainWindow::onExportFinished()
{
    hideCenterProgress();
    ui->exportBtn->setEnabled(true);
    ui->progressBar->setValue(100);

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("导出完成"));

    QMessageBox::information(this, QStringLiteral("导出成功"),
                             QStringLiteral("数据已导出到选择的目录。"));
}

void MainWindow::onAllFilesProcessed()
{
}

void MainWindow::onFirstPage()
{
    if (m_currentPage != 1) {
        m_currentPage = 1;
        displayPage(m_currentPage);
    }
}

void MainWindow::onPrevPage()
{
    if (m_currentPage > 1) {
        m_currentPage--;
        displayPage(m_currentPage);
    }
}

void MainWindow::onNextPage()
{
    if (m_currentPage < m_totalPages) {
        m_currentPage++;
        displayPage(m_currentPage);
    }
}

void MainWindow::onLastPage()
{
    if (m_currentPage != m_totalPages) {
        m_currentPage = m_totalPages;
        displayPage(m_currentPage);
    }
}

void MainWindow::updatePageInfo()
{
    int totalCount = m_currentOrders.size();
    m_totalPages = qMax(1, (totalCount + m_pageSize - 1) / m_pageSize);
    if (m_currentPage > m_totalPages)
        m_currentPage = m_totalPages;

    ui->pageInfoLabel->setText(
        QStringLiteral("第 %1/%2 页 (共 %3 条)")
            .arg(m_currentPage)
            .arg(m_totalPages)
            .arg(totalCount));

    ui->firstPageBtn->setEnabled(m_currentPage > 1);
    ui->prevPageBtn->setEnabled(m_currentPage > 1);
    ui->nextPageBtn->setEnabled(m_currentPage < m_totalPages);
    ui->lastPageBtn->setEnabled(m_currentPage < m_totalPages);
}

void MainWindow::displayPage(int page)
{
    int totalCount = m_currentOrders.size();
    if (totalCount == 0) {
        // 清除旧数据
        QAbstractItemModel *oldModel = ui->dataTableView->model();
        ui->dataTableView->setModel(new QStandardItemModel(this));
        delete oldModel;
        updatePageInfo();
        return;
    }

    int startIdx = (page - 1) * m_pageSize;
    int endIdx = qMin(startIdx + m_pageSize, totalCount);

    auto *model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({
        QStringLiteral("运单号"),
        QStringLiteral("业务时间"),
        QStringLiteral("体积重(kg)"),
        QStringLiteral("订单客户"),
        QStringLiteral("结算对象"),
        QStringLiteral("目的省份"),
        QStringLiteral("实际重量(kg)"),
        QStringLiteral("运费(元)"),
        QStringLiteral("错误信息")
    });

    for (int i = startIdx; i < endIdx; ++i) {
        const OrderData &order = m_currentOrders[i];
        QList<QStandardItem*> row;
        row << new QStandardItem(order.waybillNo);
        row << new QStandardItem(order.businessTime.toString(QStringLiteral("yyyy-MM-dd")));
        row << new QStandardItem(QString::number(order.volumetricWeight, 'f', 2));
        row << new QStandardItem(order.customer);
        row << new QStandardItem(order.client);
        row << new QStandardItem(order.destinationProvince);
        row << new QStandardItem(QString::number(order.actualWeight, 'f', 2));
        row << new QStandardItem(order.freight > 0 ? QString::number(order.freight, 'f', 2) : QString());
        row << new QStandardItem(order.errorMessage);

        for (auto *item : row) {
            item->setEditable(false);
            item->setTextAlignment(Qt::AlignCenter);
        }
        row.last()->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        model->appendRow(row);
    }

    QAbstractItemModel *oldModel = ui->dataTableView->model();
    ui->dataTableView->setModel(model);
    delete oldModel;

    ui->dataTableView->resizeColumnsToContents();
    ui->dataTableView->horizontalHeader()->setStretchLastSection(true);

    updatePageInfo();
}

void MainWindow::updateTableView()
{
    displayPage(m_currentPage);
}

void MainWindow::updateCustomerCombo()
{
    ui->customerCombo->clear();
    ui->customerCombo->addItem(QStringLiteral("全部"));
    QStringList names = m_ruleManager->ruleNames();
    ui->customerCombo->addItems(names);
}

void MainWindow::setupBanner()
{
    m_bannerWidget = new BannerWidget(this);
    m_bannerWidget->setTexts({
        QStringLiteral("用C++打造的专业快递软件"),
        QStringLiteral("简单、易用、快速"),
        QStringLiteral("专业服务于快递网点财务运费结算")
    });
    m_bannerWidget->setInterval(3000);
    m_bannerWidget->setFixedHeight(40);

    auto *layout = new QVBoxLayout(ui->bannerPlaceholder);
    layout->addWidget(m_bannerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
}

// ============================================================================
// ����������˵���Ի���
// ============================================================================

// ============================================================================
// BiaoTouYingShe
// ============================================================================

void MainWindow::onHeaderMappingClicked()
{
    if (m_currentFilePaths.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                             QStringLiteral("\u8bf7\u5148\u5bfc\u5165 Excel \u6587\u4ef6\u3002"));
        return;
    }

    if (m_lastImportedHeaders.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("\u63d0\u793a"),
                             QStringLiteral("\u65e0\u6cd5\u8bfb\u53d6\u5bfc\u5165\u6587\u4ef6\u7684\u8868\u5934\u4fe1\u606f\u3002"));
        return;
    }

    HeaderMappingDialog dialog(m_lastImportedHeaders, m_lastColumnMapping,
                               m_lastImportedFilePath, m_tplManager, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QMap<QString, int> newMapping = dialog.mapping();

    if (newMapping == m_lastColumnMapping) {
        return;
    }

    m_lastColumnMapping = newMapping;
    m_currentOrders.clear();
    m_currentPage = 1;

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel)
        statusLabel->setText(QStringLiteral("\u6b63\u5728\u4f7f\u7528\u65b0\u6620\u5c04\u91cd\u65b0\u5bfc\u5165..."));

    ui->progressBar->setValue(0);
    ui->importBtn->setEnabled(false);
    ui->headerMappingBtn->setEnabled(false);
    ui->calculateBtn->setEnabled(false);
    ui->exportBtn->setEnabled(false);

    showCenterProgress(QStringLiteral("\u6b63\u5728\u91cd\u65b0\u5bfc\u5165..."));

    importFilesAsync(m_currentFilePaths, true, ui->sheet1OnlyCheck->isChecked());
}

void MainWindow::onRuleHelpClicked()
{
    RuleHelpDialog *dialog = new RuleHelpDialog(this);
    dialog->show();
}

// ============================================================================
// 历史记录功能
// ============================================================================

QString MainWindow::historyFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/calculation_history.json");
}

void MainWindow::saveCalculationHistory()
{
    // 构建历史记录
    CalculationHistoryRecord rec;
    rec.timestamp = QDateTime::currentDateTime();
    rec.totalCount = m_currentOrders.size();

    int errorCount = 0;
    double totalFreight = 0.0;
    for (const OrderData &order : m_currentOrders) {
        if (!order.isValid && !order.errorMessage.isEmpty()) {
            errorCount++;
        }
        totalFreight += order.freight;
    }
    rec.successCount = rec.totalCount - errorCount;
    rec.errorCount = errorCount;
    rec.totalFreight = totalFreight;
    rec.calculationMode = m_currentCalcMode;
    rec.selectedCustomer = m_currentCustomer;
    rec.threadCount = m_currentThreadCount;
    rec.avgWeightBase = m_currentAvgBase;
    rec.avgWeightUpperLimit = m_currentAvgLimit;
    rec.avgWeightIncrement = m_currentAvgStep;
    rec.avgWeightSurcharge = m_currentAvgSurcharge;

    // 规则名称列表
    rec.ruleNames = m_ruleManager->ruleNames();

    // 文件名称
    for (const QString &fp : m_currentFilePaths) {
        rec.fileNames.append(fp);
    }

    // 全局规则计数
    GlobalRules gr = m_ruleManager->globalRules();
    rec.activityRuleCount = gr.activityRules.size();
    rec.tempPriceIncreaseCount = gr.tempPriceIncreases.size();
    rec.provincePriceIncreaseCount = gr.provincePriceIncreases.size();

    // 加载已有历史记录
    QList<CalculationHistoryRecord> history = loadHistory();

    // 限制最多保留 100 条历史记录
    const int MAX_HISTORY = 100;
    while (history.size() >= MAX_HISTORY) {
        history.removeFirst();
    }
    history.append(rec);

    // 保存到文件
    QJsonArray arr;
    for (const CalculationHistoryRecord &h : history) {
        arr.append(h.toJson());
    }

    QJsonDocument doc(arr);
    QFile file(historyFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

QList<CalculationHistoryRecord> MainWindow::loadHistory() const
{
    QList<CalculationHistoryRecord> history;

    QFile file(historyFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return history;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return history;
    }

    const QJsonArray arr = doc.array();
    for (const QJsonValue &val : arr) {
        if (val.isObject()) {
            history.append(CalculationHistoryRecord::fromJson(val.toObject()));
        }
    }

    return history;
}

void MainWindow::onHistoryClicked()
{
    HistoryDialog dialog(historyFilePath(), this);
    dialog.exec();
}

void MainWindow::onChartClicked()
{
    if (m_currentOrders.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("没有可展示的数据，请先导入并计算。"));
        return;
    }

    // 过滤掉无效数据行（但保留有运费数据的行）
    QList<OrderData> validOrders;
    for (const OrderData &order : m_currentOrders) {
        // 只过滤掉完全无效且无运费的行
        if (order.freight > 0 || order.isValid || order.errorMessage.isEmpty()) {
            validOrders.append(order);
        }
    }
    // 如果过滤后为空，就使用全部数据
    if (validOrders.isEmpty()) {
        validOrders = m_currentOrders;
    }

    ChartDialog dialog(validOrders, m_currentCalcMode, this);
    dialog.exec();
}

void MainWindow::onClearClicked()
{
    if (m_isCalculating) return;

    m_currentOrders.clear();
    m_currentFilePaths.clear();
    m_currentPage = 1;
    m_totalPages = 1;
    m_isCalculating = false;
    m_lastDedupCount = 0;
    m_lastImportedHeaders.clear();
    m_lastColumnMapping.clear();
    m_lastImportedFilePath.clear();
    m_calcTotalRows = 0;

    updateTableView();
    ui->progressBar->setValue(0);

    auto *statusLabel = statusBar()->findChild<QLabel*>(QStringLiteral("statusLabel"));
    if (statusLabel) statusLabel->setText(QStringLiteral("就绪"));
    auto *recordLabel = statusBar()->findChild<QLabel*>(QStringLiteral("recordLabel"));
    if (recordLabel) recordLabel->setText(QStringLiteral("记录数: 0"));
}

void MainWindow::onCourierChanged(const QString &courier)
{
    if (courier.isEmpty())
        return;

    m_ruleManager->setCourier(courier);
    m_calculator->setDefaultPriceTable(m_ruleManager->defaultPriceTable());
    m_ruleManager->saveToFile(RuleManager::defaultFilePath());
}
