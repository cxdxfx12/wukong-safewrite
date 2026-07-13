#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QFuture>
#include <QFutureWatcher>
#include <QMap>
#include <QAtomicInt>
#include <QPointer>
#include <QElapsedTimer>
#include "DataModel/OrderData.h"
#include "DataModel/PriceTable.h"
#include "Calculation/FreightCalculator.h"
#include "Calculation/RuleManager.h"
#include "UI/BannerWidget.h"
#include "DataModel/CalculationHistory.h"
#include "DataModel/MappingTemplate.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void progressUpdated(int percent);
    void errorOccurred(const QString &message);
    void importProgress(int percent);
    void calcProgress(int percent);

private slots:
    void onImportClicked();
    void onCalculateClicked();
    void onExportClicked();
    void onQuickTestClicked();
    void onRuleManageClicked();
    void onGlobalRulesClicked();
    void updateProgress(int percent);
    void onCalculationComplete(int totalCount, int errorCount);
    void onImportFinished();
    void onExportFinished();
    void onAllFilesProcessed();
    void onCalcModeChanged(const QString &mode);
    void onAvgParamChanged();
    void onCourierChanged(const QString &courier);

    void onFirstPage();
    void onPrevPage();
    void onNextPage();
    void onLastPage();
    void onHeaderMappingClicked(); // 新增：表头映射
    void onRuleHelpClicked();  // 新增：规则说明
    void onHistoryClicked();   // 新增：历史记录
    void onClearClicked();     // 清除：重置所有数据
    void onChartClicked();     // 新增：生成图表

private:
    Ui::MainWindow *ui;
    QList<OrderData> m_currentOrders;
    QList<QString> m_currentFilePaths;
    FreightCalculator *m_calculator;
    RuleManager *m_ruleManager;
    MappingTemplateManager *m_tplManager;
    QFutureWatcher<void> *m_importWatcher;
    QFutureWatcher<void> *m_exportWatcher;
    QFutureWatcher<void> *m_batchWatcher;
    int m_totalFileCount;
    QAtomicInt m_processedFileCount;

    QElapsedTimer m_lastProgressTime;
    int m_lastProgressPercent = -1;

    int m_currentPage = 1;
    int m_pageSize = 5000;
    int m_totalPages = 1;

    QWidget *m_overlayWidget = nullptr;
    class QProgressBar *m_centerProgress = nullptr;
    class QLabel *m_centerProgressLabel = nullptr;
    BannerWidget *m_bannerWidget = nullptr;

    bool m_isCalculating = false;
    int m_calcTotalRows = 0;
    
    // 表头映射上下文（用于重新导入）
    QStringList m_lastImportedHeaders;
    QMap<QString, int> m_lastColumnMapping;
    QString m_lastImportedFilePath;
    int m_lastDedupCount = 0;  // 最近一次导入去重条数

    // 当前计算上下文（用于记录历史）
    QString m_currentCalcMode;
    QString m_currentCustomer;
    int m_currentThreadCount = 8;
    double m_currentAvgBase = 0.5;
    double m_currentAvgLimit = 1.0;
    double m_currentAvgStep = 0.1;
    double m_currentAvgSurcharge = 0.1;

    void setupConnections();
    void setupStyle();
    void setupStatusBar();
    void setupToolbarIcons();
    void updateTableView();
    void updateCustomerCombo();
    void applyLightStyle();
    void importFilesAsync(const QStringList &filePaths, bool useExistingMapping = false,
                          bool sheet1Only = true);
    void exportFilesAsync(const QString &outputDir, bool mergeExport, bool perClientExport, bool exportSummary);

    void showCenterProgress(const QString &text);
    void hideCenterProgress();
    void updateCenterProgress(int percent);

    void throttledProgress(int percent);

    void updatePageInfo();
    void displayPage(int page);
    void setupBanner();
    void saveCalculationHistory();
    QList<CalculationHistoryRecord> loadHistory() const;
    QString historyFilePath() const;
};

#endif // MAINWINDOW_H
