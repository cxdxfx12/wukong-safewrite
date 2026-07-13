#include "RuleManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

QString RuleManager::defaultFilePath()
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) {
        return QStringLiteral("rules.json");
    }
    QDir().mkpath(appDataDir);
    return appDataDir + QDir::separator() + QStringLiteral("rules.json");
}

RuleManager::RuleManager(QObject *parent)
    : QObject(parent)
{
    initRegionMappings();
}

// ===================== 客户规则CRUD =====================

void RuleManager::addCustomerRule(const CustomerRule &rule)
{
    m_rules.insert(rule.customerName, rule);
    emit rulesChanged();
}

void RuleManager::updateCustomerRule(const QString &name, const CustomerRule &rule)
{
    m_rules.insert(name, rule);
    emit rulesChanged();
}

void RuleManager::removeCustomerRule(const QString &name)
{
    m_rules.remove(name);
    emit rulesChanged();
}

CustomerRule RuleManager::getRule(const QString &name) const
{
    return m_rules.value(name);
}

QList<CustomerRule> RuleManager::allRules() const
{
    return m_rules.values();
}

QStringList RuleManager::ruleNames() const
{
    return m_rules.keys();
}

QMap<QString, CustomerRule> RuleManager::rulesMap() const
{
    return m_rules;
}

void RuleManager::saveRule(const CustomerRule &rule)
{
    m_rules.insert(rule.customerName, rule);
    emit rulesChanged();
}

// ===================== 默认报价表 =====================

void RuleManager::setDefaultPriceTable(const QList<PriceRule> &rules)
{
    m_defaultPriceTable = rules;
}

QList<PriceRule> RuleManager::defaultPriceTable() const
{
    return m_defaultPriceTable;
}

// ===================== 全局规则 =====================

GlobalRules RuleManager::globalRules() const
{
    return m_globalRules;
}

void RuleManager::setGlobalRules(const GlobalRules &rules)
{
    m_globalRules = rules;
    emit rulesChanged();
}

QList<PriceIncreaseRule> RuleManager::activityRules() const
{
    return m_globalRules.activityRules;
}

void RuleManager::setActivityRules(const QList<PriceIncreaseRule> &rules)
{
    m_globalRules.activityRules = rules;
    emit rulesChanged();
}

void RuleManager::addActivityRule(const PriceIncreaseRule &rule)
{
    m_globalRules.activityRules.append(rule);
    emit rulesChanged();
}

void RuleManager::removeActivityRule(int index)
{
    if (index >= 0 && index < m_globalRules.activityRules.size()) {
        m_globalRules.activityRules.removeAt(index);
        emit rulesChanged();
    }
}

QList<PriceIncreaseRule> RuleManager::tempPriceIncreases() const
{
    return m_globalRules.tempPriceIncreases;
}

void RuleManager::setTempPriceIncreases(const QList<PriceIncreaseRule> &rules)
{
    m_globalRules.tempPriceIncreases = rules;
    emit rulesChanged();
}

void RuleManager::addTempPriceIncrease(const PriceIncreaseRule &rule)
{
    m_globalRules.tempPriceIncreases.append(rule);
    emit rulesChanged();
}

void RuleManager::removeTempPriceIncrease(int index)
{
    if (index >= 0 && index < m_globalRules.tempPriceIncreases.size()) {
        m_globalRules.tempPriceIncreases.removeAt(index);
        emit rulesChanged();
    }
}

QList<PriceIncreaseRule> RuleManager::provincePriceIncreases() const
{
    return m_globalRules.provincePriceIncreases;
}

void RuleManager::setProvincePriceIncreases(const QList<PriceIncreaseRule> &rules)
{
    m_globalRules.provincePriceIncreases = rules;
    emit rulesChanged();
}

void RuleManager::addProvincePriceIncrease(const PriceIncreaseRule &rule)
{
    m_globalRules.provincePriceIncreases.append(rule);
    emit rulesChanged();
}

void RuleManager::removeProvincePriceIncrease(int index)
{
    if (index >= 0 && index < m_globalRules.provincePriceIncreases.size()) {
        m_globalRules.provincePriceIncreases.removeAt(index);
        emit rulesChanged();
    }
}

double RuleManager::noWeightDefaultPrice() const
{
    return m_globalRules.noWeightDefaultPrice;
}

void RuleManager::setNoWeightDefaultPrice(double price)
{
    m_globalRules.noWeightDefaultPrice = price;
    emit rulesChanged();
}

// ===================== 快递公司管理 =====================

QString RuleManager::courierName() const
{
    return m_currentCourier.isEmpty() ? QStringLiteral("申通") : m_currentCourier;
}

QStringList RuleManager::courierNames() const
{
    return m_allPriceTables.keys();
}

QList<PriceRule> RuleManager::priceTableForCourier(const QString &courierName) const
{
    if (m_allPriceTables.contains(courierName))
        return m_allPriceTables[courierName];
    return m_defaultPriceTable; // 回退
}

void RuleManager::setCourier(const QString &name)
{
    if (m_allPriceTables.contains(name)) {
        m_currentCourier = name;
        m_defaultPriceTable = m_allPriceTables[name];
        emit rulesChanged();
    }
}

// ===================== 初始化默认报价表 =====================

void RuleManager::initDefaultPriceTable()
{
    initAllPriceTables();
    setCourier(QStringLiteral("申通"));
}

void RuleManager::initAllPriceTables()
{
    m_allPriceTables.clear();

    // 区域省份定义
    const QStringList zone1 = {QStringLiteral("江苏"), QStringLiteral("浙江"),
                               QStringLiteral("安徽"), QStringLiteral("上海")};
    const QStringList zone2 = {QStringLiteral("山东"), QStringLiteral("广东"), QStringLiteral("福建"),
                               QStringLiteral("北京"), QStringLiteral("河南"), QStringLiteral("湖北"),
                               QStringLiteral("湖南"), QStringLiteral("江西"), QStringLiteral("天津"),
                               QStringLiteral("河北")};
    const QStringList zone3 = {QStringLiteral("山西"), QStringLiteral("广西"), QStringLiteral("四川"),
                               QStringLiteral("重庆"), QStringLiteral("陕西"), QStringLiteral("贵州"),
                               QStringLiteral("辽宁"), QStringLiteral("吉林"), QStringLiteral("黑龙江"),
                               QStringLiteral("云南")};
    const QStringList zone4 = {QStringLiteral("海南"), QStringLiteral("甘肃"), QStringLiteral("青海"),
                               QStringLiteral("内蒙古"), QStringLiteral("宁夏")};

    // 价格结构（每快递一套）:
    // {courierName,
    //  一区{seg0_0.5, seg0.5_1, seg1_2, seg2_3, add3_30, add30_},
    //  二区{seg0_0.5, seg0.5_1, seg1_2, seg2_3, add3_30, add30_},
    //  三区{seg0_0.5, seg0.5_1, seg1_2, seg2_3, add3_30, add30_},
    //  四区{seg0_0.5, seg0.5_1, seg1_2, seg2_3, add3_30, add30_},
    //  五区新疆, 五区西藏}
    //
    // 每区参数: {p05, p1, p2, p3, base3, add3_30, base30, add30}
    // p05=0-0.5首重价, p1=0.5-1首重价, p2=1-2首重价, p3=2-3首重价
    // base3=3-30的3kg基数价, add3_30=3-30续重价
    // base30=30+的30kg基数价, add30=30+续重价

    struct ZonePrices {
        double p05, p1, p2, p3, base3, add3_30, base30, add30;
    };

    struct CourierPrices {
        QString name;
        ZonePrices z1, z2, z3, z4;
        // 五区新疆
        double xj_p05, xj_p1, xj_p2, xj_p3, xj_base3, xj_add3_30, xj_base30, xj_add30;
        // 五区西藏
        double xz_p05, xz_p1, xz_p2, xz_p3, xz_base3, xz_add3_30, xz_base30, xz_add30;
    };

    const QList<CourierPrices> couriers = {
        // 申通 — 现有价格（基准）
        {QStringLiteral("申通"),
         {2.26,2.46,3.56,4.76,4.76,0.8,3.86,0.8},
         {2.26,2.46,3.56,4.76,4.76,1.1,4.06,1.3},
         {2.26,2.46,3.56,4.76,4.76,1.5,4.06,1.6},
         {2.56,3.56,4.06,5.06,5.06,2.5,4.06,4.3},
         10,13,20,25,25,15,15,15,
         13,15,25,30,30,15,15,15},

        // 中通 — 首重略低，续重持平或略低
        {QStringLiteral("中通"),
         {2.20,2.40,3.50,4.70,4.70,0.8,3.80,0.8},
         {2.20,2.40,3.50,4.70,4.70,1.1,4.00,1.2},
         {2.20,2.40,3.50,4.70,4.70,1.4,4.00,1.5},
         {2.50,3.50,4.00,5.00,5.00,2.4,4.00,4.0},
         10,13,20,25,25,14,15,14,
         13,15,25,30,30,14,15,14},

        // 圆通 — 首重接近中通，二区续重略低
        {QStringLiteral("圆通"),
         {2.20,2.40,3.50,4.70,4.70,0.8,3.80,0.8},
         {2.20,2.40,3.50,4.70,4.70,1.0,4.00,1.2},
         {2.20,2.40,3.50,4.70,4.70,1.4,4.00,1.5},
         {2.50,3.50,4.00,5.00,5.00,2.4,4.00,4.0},
         10,13,20,25,25,15,15,15,
         13,15,25,30,30,15,15,15},

        // 韵达 — 经济型，各段略低
        {QStringLiteral("韵达"),
         {2.10,2.30,3.40,4.60,4.60,0.7,3.70,0.7},
         {2.10,2.30,3.40,4.60,4.60,1.0,3.90,1.2},
         {2.10,2.30,3.40,4.60,4.60,1.3,3.90,1.5},
         {2.40,3.40,3.90,4.90,4.90,2.3,3.90,4.0},
         10,13,20,25,25,14,15,14,
         13,15,25,30,30,14,15,14},

        // 顺丰 — 高端，价格显著更高
        {QStringLiteral("顺丰"),
         {12.0,14.0,18.0,22.0,22.0,5.0,20.0,5.0},
         {12.0,14.0,18.0,23.0,23.0,6.0,22.0,8.0},
         {13.0,15.0,20.0,25.0,25.0,8.0,24.0,10.0},
         {15.0,18.0,24.0,30.0,30.0,12.0,28.0,15.0},
         20,25,35,45,45,20,40,25,
         23,30,40,50,50,20,45,25},

        // 京东 — 品质型，价格在通达系和顺丰之间
        {QStringLiteral("京东"),
         {2.30,2.50,3.60,4.80,4.80,0.9,3.90,0.9},
         {2.30,2.50,3.60,4.80,4.80,1.1,4.10,1.3},
         {2.30,2.50,3.60,4.80,4.80,1.5,4.10,1.6},
         {2.60,3.60,4.10,5.10,5.10,2.5,4.10,4.3},
         10,13,20,25,25,15,15,15,
         13,15,25,30,30,15,15,15},
    };

    // 辅助函数：为指定区域的省份生成PriceRule
    auto buildZone = [](const QString &region, const QStringList &provinces, const ZonePrices &zp) -> QList<PriceRule> {
        QList<PriceRule> rules;
        for (const QString &prov : provinces) {
            PriceRule rule;
            rule.region = region;
            rule.province = prov;
            rule.segments.append({0, 0.5, zp.p05, 0, false, false});
            rule.segments.append({0.5, 1, zp.p1, 0, false, false});
            rule.segments.append({1, 2, zp.p2, 0, false, false});
            rule.segments.append({2, 3, zp.p3, 0, false, false});
            rule.segments.append({3, 30, zp.base3, zp.add3_30, false, false});
            rule.segments.append({30, -1, zp.base30, zp.add30, false, false});
            rules.append(rule);
        }
        return rules;
    };

    for (const CourierPrices &cp : couriers) {
        QList<PriceRule> table;

        table.append(buildZone(QStringLiteral("一区"), zone1, cp.z1));
        table.append(buildZone(QStringLiteral("二区"), zone2, cp.z2));
        table.append(buildZone(QStringLiteral("三区"), zone3, cp.z3));
        table.append(buildZone(QStringLiteral("四区"), zone4, cp.z4));

        // 五区：新疆
        {
            PriceRule xj;
            xj.region = QStringLiteral("五区");
            xj.province = QStringLiteral("新疆");
            xj.segments.append({0, 0.5, cp.xj_p05, 0, false, false});
            xj.segments.append({0.5, 1, cp.xj_p1, 0, false, false});
            xj.segments.append({1, 2, cp.xj_p2, 0, false, false});
            xj.segments.append({2, 3, cp.xj_p3, 0, false, false});
            xj.segments.append({3, 30, cp.xj_base3, cp.xj_add3_30, false, false});
            xj.segments.append({30, -1, cp.xj_base30, cp.xj_add30, false, false});
            table.append(xj);
        }
        // 五区：西藏
        {
            PriceRule xz;
            xz.region = QStringLiteral("五区");
            xz.province = QStringLiteral("西藏");
            xz.segments.append({0, 0.5, cp.xz_p05, 0, false, false});
            xz.segments.append({0.5, 1, cp.xz_p1, 0, false, false});
            xz.segments.append({1, 2, cp.xz_p2, 0, false, false});
            xz.segments.append({2, 3, cp.xz_p3, 0, false, false});
            xz.segments.append({3, 30, cp.xz_base3, cp.xz_add3_30, false, false});
            xz.segments.append({30, -1, cp.xz_base30, cp.xz_add30, false, false});
            table.append(xz);
        }

        m_allPriceTables.insert(cp.name, table);
    }
}

// ===================== 区域映射初始化 =====================

void RuleManager::initRegionMappings()
{
    m_regionMappings.clear();

    m_regionMappings.append({QStringLiteral("一区"), {QStringLiteral("江苏"), QStringLiteral("浙江"),
                                                       QStringLiteral("安徽"), QStringLiteral("上海")}});
    m_regionMappings.append({QStringLiteral("二区"), {QStringLiteral("山东"), QStringLiteral("广东"), QStringLiteral("福建"),
                                                       QStringLiteral("北京"), QStringLiteral("河南"), QStringLiteral("湖北"),
                                                       QStringLiteral("湖南"), QStringLiteral("江西"), QStringLiteral("天津"),
                                                       QStringLiteral("河北")}});
    m_regionMappings.append({QStringLiteral("三区"), {QStringLiteral("山西"), QStringLiteral("广西"), QStringLiteral("四川"),
                                                       QStringLiteral("重庆"), QStringLiteral("陕西"), QStringLiteral("贵州"),
                                                       QStringLiteral("辽宁"), QStringLiteral("吉林"), QStringLiteral("黑龙江"),
                                                       QStringLiteral("云南")}});
    m_regionMappings.append({QStringLiteral("四区"), {QStringLiteral("海南"), QStringLiteral("甘肃"), QStringLiteral("青海"),
                                                       QStringLiteral("内蒙古"), QStringLiteral("宁夏")}});
    m_regionMappings.append({QStringLiteral("五区"), {QStringLiteral("新疆"), QStringLiteral("西藏")}});
}

// ===================== 持久化 =====================

namespace {

QJsonObject weightSegmentToJson(const WeightSegment &seg)
{
    QJsonObject obj;
    obj[QStringLiteral("minWeight")] = seg.minWeight;
    obj[QStringLiteral("maxWeight")] = seg.maxWeight;
    obj[QStringLiteral("firstWeightPrice")] = seg.firstWeightPrice;
    obj[QStringLiteral("additionalPrice")] = seg.additionalPrice;
    obj[QStringLiteral("isFullAdditional")] = seg.isFullAdditional;
    obj[QStringLiteral("isHundredGram")] = seg.isHundredGram;
    return obj;
}

WeightSegment weightSegmentFromJson(const QJsonObject &obj)
{
    WeightSegment seg;
    seg.minWeight = obj[QStringLiteral("minWeight")].toDouble(0);
    seg.maxWeight = obj[QStringLiteral("maxWeight")].toDouble(-1);
    seg.firstWeightPrice = obj[QStringLiteral("firstWeightPrice")].toDouble(0);
    seg.additionalPrice = obj[QStringLiteral("additionalPrice")].toDouble(0);
    seg.isFullAdditional = obj[QStringLiteral("isFullAdditional")].toBool(false);
    seg.isHundredGram = obj[QStringLiteral("isHundredGram")].toBool(false);
    return seg;
}

QJsonObject priceRuleToJson(const PriceRule &rule)
{
    QJsonObject obj;
    obj[QStringLiteral("region")] = rule.region;
    obj[QStringLiteral("province")] = rule.province;
    QJsonArray segArr;
    for (const WeightSegment &seg : rule.segments) {
        segArr.append(weightSegmentToJson(seg));
    }
    obj[QStringLiteral("segments")] = segArr;
    return obj;
}

PriceRule priceRuleFromJson(const QJsonObject &obj)
{
    PriceRule rule;
    rule.region = obj[QStringLiteral("region")].toString();
    rule.province = obj[QStringLiteral("province")].toString();
    QJsonArray segArr = obj[QStringLiteral("segments")].toArray();
    for (const QJsonValue &val : segArr) {
        rule.segments.append(weightSegmentFromJson(val.toObject()));
    }
    return rule;
}
// unified price-increase JSON
QJsonObject ruleToJson(const PriceIncreaseRule &rule)
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = rule.name;
    obj[QStringLiteral("mode")] = static_cast<int>(rule.mode);
    obj[QStringLiteral("amount")] = rule.amount;
    if (rule.startTime.isValid())
        obj[QStringLiteral("startTime")] = rule.startTime.toString(Qt::ISODate);
    if (rule.endTime.isValid())
        obj[QStringLiteral("endTime")] = rule.endTime.toString(Qt::ISODate);
    if (!rule.province.isEmpty())
        obj[QStringLiteral("province")] = rule.province;
    obj[QStringLiteral("isActive")] = rule.isActive;
    return obj;
}

PriceIncreaseRule ruleFromJson(const QJsonObject &obj)
{
    PriceIncreaseRule rule;
    rule.name = obj[QStringLiteral("name")].toString();
    rule.province = obj[QStringLiteral("province")].toString();
    rule.isActive = obj[QStringLiteral("isActive")].toBool(true);
    rule.startTime = QDateTime::fromString(obj[QStringLiteral("startTime")].toString(), Qt::ISODate);
    rule.endTime = QDateTime::fromString(obj[QStringLiteral("endTime")].toString(), Qt::ISODate);
    if (obj.contains(QStringLiteral("mode"))) {
        rule.mode = static_cast<IncreaseMode>(obj[QStringLiteral("mode")].toInt(0));
        rule.amount = obj[QStringLiteral("amount")].toDouble(0.0);
    } else if (obj.contains(QStringLiteral("isFixedAmount"))) {
        bool isFixed = obj[QStringLiteral("isFixedAmount")].toBool();
        rule.mode = isFixed ? IncreaseMode::PerTicketFixed : IncreaseMode::PerTicketPercent;
        rule.amount = isFixed ? obj[QStringLiteral("increaseAmount")].toDouble(0.0)
                              : obj[QStringLiteral("increaseRate")].toDouble(0.0);
    } else if (obj.contains(QStringLiteral("discountRate"))) {
        rule.mode = IncreaseMode::PerTicketPercent;
        double dr = obj[QStringLiteral("discountRate")].toDouble(1.0);
        rule.amount = (dr > 0 && dr != 1.0) ? (dr - 1.0) : 0.0;
    } else if (obj.contains(QStringLiteral("increaseAmount"))) {
        rule.mode = IncreaseMode::PerTicketFixed;
        rule.amount = obj[QStringLiteral("increaseAmount")].toDouble(0.0);
    }
    return rule;
}

QJsonObject customerRuleToJson(const CustomerRule &rule)
{
    QJsonObject obj;
    obj[QStringLiteral("customerName")] = rule.customerName;
    obj[QStringLiteral("mappingHeader")] = rule.mappingHeader;
    obj[QStringLiteral("calculationMode")] = rule.calculationMode;
    obj[QStringLiteral("useDefaultPrice")] = rule.useDefaultPrice;
    obj[QStringLiteral("courier")] = rule.courier;

    QJsonArray customArr;
    for (const PriceRule &pr : rule.customPriceRules) {
        customArr.append(priceRuleToJson(pr));
    }
    obj[QStringLiteral("customPriceRules")] = customArr;

    return obj;
}

CustomerRule customerRuleFromJson(const QJsonObject &obj)
{
    CustomerRule rule;
    rule.customerName = obj[QStringLiteral("customerName")].toString();
    rule.mappingHeader = obj[QStringLiteral("mappingHeader")].toString();
    rule.calculationMode = obj[QStringLiteral("calculationMode")].toString();
    rule.useDefaultPrice = obj[QStringLiteral("useDefaultPrice")].toBool(true);
    rule.courier = obj[QStringLiteral("courier")].toString();

    QJsonArray customArr = obj[QStringLiteral("customPriceRules")].toArray();
    for (const QJsonValue &val : customArr) {
        rule.customPriceRules.append(priceRuleFromJson(val.toObject()));
    }

    return rule;
}

QJsonObject globalRulesToJson(const GlobalRules &rules)
{
    QJsonObject obj;
    obj[QStringLiteral("noWeightDefaultPrice")] = rules.noWeightDefaultPrice;

    QJsonArray activityArr;
    for (const PriceIncreaseRule &ar : rules.activityRules) {
        activityArr.append(ruleToJson(ar));
    }
    obj[QStringLiteral("activityRules")] = activityArr;

    QJsonArray tempArr;
    for (const PriceIncreaseRule &tr : rules.tempPriceIncreases) {
        tempArr.append(ruleToJson(tr));
    }
    obj[QStringLiteral("tempPriceIncreases")] = tempArr;

    QJsonArray provArr;
    for (const PriceIncreaseRule &pr : rules.provincePriceIncreases) {
        provArr.append(ruleToJson(pr));
    }
    obj[QStringLiteral("provincePriceIncreases")] = provArr;

    // 拉均重配置
    obj[QStringLiteral("avgWeightBase")] = rules.avgWeightBase;
    obj[QStringLiteral("avgWeightUpperLimit")] = rules.avgWeightUpperLimit;
    obj[QStringLiteral("avgWeightIncrement")] = rules.avgWeightIncrement;
    obj[QStringLiteral("avgWeightSurcharge")] = rules.avgWeightSurcharge;

    return obj;
}

GlobalRules globalRulesFromJson(const QJsonObject &obj)
{
    GlobalRules rules;
    rules.noWeightDefaultPrice = obj[QStringLiteral("noWeightDefaultPrice")].toDouble(0);

    QJsonArray activityArr = obj[QStringLiteral("activityRules")].toArray();
    for (const QJsonValue &val : activityArr) {
        rules.activityRules.append(ruleFromJson(val.toObject()));
    }

    QJsonArray tempArr = obj[QStringLiteral("tempPriceIncreases")].toArray();
    for (const QJsonValue &val : tempArr) {
        rules.tempPriceIncreases.append(ruleFromJson(val.toObject()));
    }

    QJsonArray provArr = obj[QStringLiteral("provincePriceIncreases")].toArray();
    for (const QJsonValue &val : provArr) {
        rules.provincePriceIncreases.append(ruleFromJson(val.toObject()));
    }

    // 拉均重配置
    rules.avgWeightBase = obj[QStringLiteral("avgWeightBase")].toDouble(0.5);
    rules.avgWeightUpperLimit = obj[QStringLiteral("avgWeightUpperLimit")].toDouble(1.0);
    rules.avgWeightIncrement = obj[QStringLiteral("avgWeightIncrement")].toDouble(0.1);
    rules.avgWeightSurcharge = obj[QStringLiteral("avgWeightSurcharge")].toDouble(0.1);

    return rules;
}

} // anonymous namespace

bool RuleManager::saveToFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);

    QJsonObject root;

    // 保存默认报价表
    QJsonArray priceArr;
    for (const PriceRule &pr : m_defaultPriceTable) {
        priceArr.append(priceRuleToJson(pr));
    }
    root[QStringLiteral("defaultPriceTable")] = priceArr;

    // 保存全局规则
    root[QStringLiteral("globalRules")] = globalRulesToJson(m_globalRules);

    // 保存客户规则
    QJsonArray rulesArr;
    for (const CustomerRule &rule : m_rules) {
        rulesArr.append(customerRuleToJson(rule));
    }
    root[QStringLiteral("customerRules")] = rulesArr;

    // 保存区域映射
    QJsonArray regionArr;
    for (const RegionMapping &rm : m_regionMappings) {
        QJsonObject obj;
        obj[QStringLiteral("region")] = rm.region;
        obj[QStringLiteral("provinces")] = QJsonArray::fromStringList(rm.provinces);
        regionArr.append(obj);
    }
    root[QStringLiteral("regionMappings")] = regionArr;

    // 保存当前快递公司
    root[QStringLiteral("courier")] = m_currentCourier.isEmpty() ? QStringLiteral("申通") : m_currentCourier;

    QJsonDocument doc(root);

    // 原子写入：先写临时文件，再重命名替换原文件
    QString tempPath = filePath + QStringLiteral(".tmp");
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    qint64 written = tempFile.write(doc.toJson(QJsonDocument::Indented));
    if (written < 0) {
        tempFile.close();
        tempFile.remove();
        return false;
    }
    tempFile.flush();
    tempFile.close();

    // 备份旧文件
    QFile oldFile(filePath);
    if (oldFile.exists()) {
        QString backupPath = filePath + QStringLiteral(".bak");
        QFile::remove(backupPath);
        oldFile.rename(backupPath);
    }

    // 原子重命名
    if (!QFile::rename(tempPath, filePath)) {
        // 重命名失败，尝试恢复备份
        QFile::rename(filePath + QStringLiteral(".bak"), filePath);
        return false;
    }

    // 删除备份
    QFile::remove(filePath + QStringLiteral(".bak"));
    return true;
}

bool RuleManager::loadFromFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        // 备份损坏的文件，便于后续恢复
        QString backupPath = filePath + QStringLiteral(".corrupt");
        QFile::remove(backupPath);
        QFile::rename(filePath, backupPath);
        return false;
    }

    QJsonObject root = doc.object();

    // 加载默认报价表
    m_defaultPriceTable.clear();
    QJsonArray priceArr = root[QStringLiteral("defaultPriceTable")].toArray();
    for (const QJsonValue &val : priceArr) {
        m_defaultPriceTable.append(priceRuleFromJson(val.toObject()));
    }

    // 加载全局规则
    m_globalRules = globalRulesFromJson(root[QStringLiteral("globalRules")].toObject());

    // 加载客户规则
    m_rules.clear();
    QJsonArray rulesArr = root[QStringLiteral("customerRules")].toArray();
    for (const QJsonValue &val : rulesArr) {
        CustomerRule rule = customerRuleFromJson(val.toObject());
        m_rules.insert(rule.customerName, rule);
    }

    // 加载区域映射
    m_regionMappings.clear();
    QJsonArray regionArr = root[QStringLiteral("regionMappings")].toArray();
    for (const QJsonValue &val : regionArr) {
        QJsonObject obj = val.toObject();
        RegionMapping rm;
        rm.region = obj[QStringLiteral("region")].toString();
        QJsonArray provArr = obj[QStringLiteral("provinces")].toArray();
        for (const QJsonValue &pv : provArr) {
            rm.provinces.append(pv.toString());
        }
        m_regionMappings.append(rm);
    }

    // 加载快递公司选择
    QString savedCourier = root[QStringLiteral("courier")].toString(QStringLiteral("申通"));
    if (m_allPriceTables.isEmpty()) {
        initAllPriceTables();
    }
    setCourier(savedCourier);

    emit rulesChanged();
    return true;
}
