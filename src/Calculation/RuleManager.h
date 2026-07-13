#ifndef RULEMANAGER_H
#define RULEMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "DataModel/PriceTable.h"

class RuleManager : public QObject {
    Q_OBJECT
public:
    explicit RuleManager(QObject *parent = nullptr);

    // 客户规则CRUD
    void addCustomerRule(const CustomerRule &rule);
    void updateCustomerRule(const QString &name, const CustomerRule &rule);
    void removeCustomerRule(const QString &name);
    CustomerRule getRule(const QString &name) const;
    QList<CustomerRule> allRules() const;
    QStringList ruleNames() const;
    QMap<QString, CustomerRule> rulesMap() const;
    void saveRule(const CustomerRule &rule);

    // 默认报价表
    void setDefaultPriceTable(const QList<PriceRule> &rules);
    QList<PriceRule> defaultPriceTable() const;

    // 快递公司管理
    QString courierName() const;
    QStringList courierNames() const;
    void setCourier(const QString &name);
    QList<PriceRule> priceTableForCourier(const QString &courierName) const;

    // 全局规则
    GlobalRules globalRules() const;
    void setGlobalRules(const GlobalRules &rules);

    // 全局加价规则 — 统一类型
    QList<PriceIncreaseRule> activityRules() const;
    void setActivityRules(const QList<PriceIncreaseRule> &rules);
    void addActivityRule(const PriceIncreaseRule &rule);
    void removeActivityRule(int index);

    QList<PriceIncreaseRule> tempPriceIncreases() const;
    void setTempPriceIncreases(const QList<PriceIncreaseRule> &rules);
    void addTempPriceIncrease(const PriceIncreaseRule &rule);
    void removeTempPriceIncrease(int index);

    QList<PriceIncreaseRule> provincePriceIncreases() const;
    void setProvincePriceIncreases(const QList<PriceIncreaseRule> &rules);
    void addProvincePriceIncrease(const PriceIncreaseRule &rule);
    void removeProvincePriceIncrease(int index);

    // 无重量默认价格
    double noWeightDefaultPrice() const;
    void setNoWeightDefaultPrice(double price);

    // 初始化默认报价表数据
    void initDefaultPriceTable();
    void initAllPriceTables();

    // 持久化
    bool saveToFile(const QString &filePath);
    bool loadFromFile(const QString &filePath);

    // 获取默认规则文件路径（应用数据目录下的 rules.json）
    static QString defaultFilePath();

signals:
    void rulesChanged();

private:
    QMap<QString, CustomerRule> m_rules;
    QList<PriceRule> m_defaultPriceTable;
    GlobalRules m_globalRules;
    QList<RegionMapping> m_regionMappings;

    // 多套快递报价表
    QMap<QString, QList<PriceRule>> m_allPriceTables;
    QString m_currentCourier;

    void initRegionMappings();
};

#endif // RULEMANAGER_H
