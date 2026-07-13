#ifndef CUSTOMERSTATS_H
#define CUSTOMERSTATS_H

#include <QString>
#include <QList>
#include <QMap>
#include <algorithm>
#include "DataModel/OrderData.h"

struct CustomerStats {
    QString customerName;
    int shipmentCount = 0;
    double totalFreight = 0.0;
    double totalWeight = 0.0;      // 计费重量 (max of actual & volumetric)
    double totalActualWeight = 0.0;
    double totalVolumetricWeight = 0.0;

    double avgWeight() const {
        return shipmentCount > 0 ? totalWeight / shipmentCount : 0.0;
    }
    double avgFreight() const {
        return shipmentCount > 0 ? totalFreight / shipmentCount : 0.0;
    }
    double avgActualWeight() const {
        return shipmentCount > 0 ? totalActualWeight / shipmentCount : 0.0;
    }

    // 按运费降序排序
    static bool sortByFreight(const CustomerStats &a, const CustomerStats &b) {
        return a.totalFreight > b.totalFreight;
    }
    // 按件数降序排序
    static bool sortByCount(const CustomerStats &a, const CustomerStats &b) {
        return a.shipmentCount > b.shipmentCount;
    }
};

// 从订单列表计算客户统计数据（按结算对象/客户分组）
inline QList<CustomerStats> computeCustomerStats(const QList<OrderData> &orders)
{
    QMap<QString, CustomerStats> statsMap;

    for (const OrderData &order : orders) {
        QString client = order.client.isEmpty() ? order.customer : order.client;
        if (client.isEmpty())
            client = QStringLiteral("未知客户");

        CustomerStats &s = statsMap[client];
        s.customerName = client;
        s.shipmentCount++;
        s.totalFreight += order.freight;
        double billableWeight = std::max(order.actualWeight, order.volumetricWeight);
        s.totalWeight += billableWeight;
        s.totalActualWeight += order.actualWeight;
        s.totalVolumetricWeight += order.volumetricWeight;
    }

    QList<CustomerStats> result = statsMap.values();
    return result;
}

// 从订单列表计算店铺统计数据（按订单客户/店铺分组）
inline QList<CustomerStats> computeStoreStats(const QList<OrderData> &orders)
{
    QMap<QString, CustomerStats> statsMap;

    for (const OrderData &order : orders) {
        QString store = order.customer;  // 订单客户 = 店铺
        if (store.isEmpty())
            store = QStringLiteral("未知店铺");

        CustomerStats &s = statsMap[store];
        s.customerName = store;
        s.shipmentCount++;
        s.totalFreight += order.freight;
        double billableWeight = std::max(order.actualWeight, order.volumetricWeight);
        s.totalWeight += billableWeight;
        s.totalActualWeight += order.actualWeight;
        s.totalVolumetricWeight += order.volumetricWeight;
    }

    QList<CustomerStats> result = statsMap.values();
    return result;
}

#endif // CUSTOMERSTATS_H
