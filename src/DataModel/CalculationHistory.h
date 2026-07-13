#ifndef CALCULATIONHISTORY_H
#define CALCULATIONHISTORY_H

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

struct CalculationHistoryRecord {
    QDateTime timestamp;
    QStringList fileNames;
    int totalCount = 0;
    int successCount = 0;
    int errorCount = 0;
    QString calculationMode;
    QString selectedCustomer;
    int threadCount = 8;
    double totalFreight = 0.0;
    QStringList ruleNames;

    // 拉均重参数
    double avgWeightBase = 0.5;
    double avgWeightUpperLimit = 1.0;
    double avgWeightIncrement = 0.1;
    double avgWeightSurcharge = 0.1;

    // 全局规则计数
    int activityRuleCount = 0;
    int tempPriceIncreaseCount = 0;
    int provincePriceIncreaseCount = 0;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["timestamp"] = timestamp.toString(Qt::ISODate);
        QJsonArray arr;
        for (const QString &f : fileNames) arr.append(f);
        obj["fileNames"] = arr;
        obj["totalCount"] = totalCount;
        obj["successCount"] = successCount;
        obj["errorCount"] = errorCount;
        obj["calculationMode"] = calculationMode;
        obj["selectedCustomer"] = selectedCustomer;
        obj["threadCount"] = threadCount;
        obj["totalFreight"] = totalFreight;
        QJsonArray rulesArr;
        for (const QString &r : ruleNames) rulesArr.append(r);
        obj["ruleNames"] = rulesArr;
        obj["avgWeightBase"] = avgWeightBase;
        obj["avgWeightUpperLimit"] = avgWeightUpperLimit;
        obj["avgWeightIncrement"] = avgWeightIncrement;
        obj["avgWeightSurcharge"] = avgWeightSurcharge;
        obj["activityRuleCount"] = activityRuleCount;
        obj["tempPriceIncreaseCount"] = tempPriceIncreaseCount;
        obj["provincePriceIncreaseCount"] = provincePriceIncreaseCount;
        return obj;
    }

    static CalculationHistoryRecord fromJson(const QJsonObject &obj) {
        CalculationHistoryRecord rec;
        rec.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        const QJsonArray arr = obj["fileNames"].toArray();
        for (const QJsonValue &v : arr) rec.fileNames.append(v.toString());
        rec.totalCount = obj["totalCount"].toInt();
        rec.successCount = obj["successCount"].toInt();
        rec.errorCount = obj["errorCount"].toInt();
        rec.calculationMode = obj["calculationMode"].toString();
        rec.selectedCustomer = obj["selectedCustomer"].toString();
        rec.threadCount = obj["threadCount"].toInt();
        rec.totalFreight = obj["totalFreight"].toDouble();
        const QJsonArray rulesArr = obj["ruleNames"].toArray();
        for (const QJsonValue &v : rulesArr) rec.ruleNames.append(v.toString());
        rec.avgWeightBase = obj["avgWeightBase"].toDouble(0.5);
        rec.avgWeightUpperLimit = obj["avgWeightUpperLimit"].toDouble(1.0);
        rec.avgWeightIncrement = obj["avgWeightIncrement"].toDouble(0.1);
        rec.avgWeightSurcharge = obj["avgWeightSurcharge"].toDouble(0.1);
        rec.activityRuleCount = obj["activityRuleCount"].toInt();
        rec.tempPriceIncreaseCount = obj["tempPriceIncreaseCount"].toInt();
        rec.provincePriceIncreaseCount = obj["provincePriceIncreaseCount"].toInt();
        return rec;
    }
};

#endif // CALCULATIONHISTORY_H
