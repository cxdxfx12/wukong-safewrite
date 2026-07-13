#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QPushButton>
#include <QList>
#include "DataModel/CustomerStats.h"

class StatCardWidget;
class DonutChartWidget;
class BarChartWidget;

class ChartDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChartDialog(const QList<OrderData> &orders,
                         const QString &calcMode = QString(),
                         QWidget *parent = nullptr);

private slots:
    void onExportImage();

private:
    QTabWidget *m_tabWidget;
    QList<CustomerStats> m_stats;      // 按结算客户分组
    QList<CustomerStats> m_storeStats; // 按店铺(订单客户)分组
    QString m_calcMode;

    // 概览卡片
    StatCardWidget *m_cardTotal;
    StatCardWidget *m_cardFreight;
    StatCardWidget *m_cardAvgWeight;
    StatCardWidget *m_cardStores;

    void setupUi();
    QWidget* createOverviewTab();
    QWidget* createCountTab();
    QWidget* createFreightTab();
    QWidget* createStoreTab();  // 原 averageTab → 店铺数据

    void exportWidgetToImage(QWidget *widget, const QString &filePath);
};

#endif // CHARTDIALOG_H
