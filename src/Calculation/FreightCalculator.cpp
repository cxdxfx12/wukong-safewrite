#include "FreightCalculator.h"
#include "ThreadPool.h"
#include "DataModel/CalculationRule.h"
#include "Utils/ProvinceUtils.h"
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <cmath>
#include <thread>
#include <vector>

FreightCalculator::FreightCalculator(QObject *parent)
    : QObject(parent)
{
}

static bool fuzzyProvinceMatch(const QString &a, const QString &b)
{
    if (a.size() >= 2 && b.size() >= 2 && a.left(2) == b.left(2)) return true;
    return false;
}

void FreightCalculator::buildRuleCache(const CustomerRule &rule)
{
    QMutexLocker locker(&m_cacheMutex);
    if (!m_defaultCacheBuilt) {
        m_cachedDefaultRules.clear();
        for (const PriceRule &pr : m_defaultPriceTable) {
            QString key = ProvinceUtils::normalize(pr.province);
            if (!key.isEmpty())
                m_cachedDefaultRules.insert(key, pr);
        }
        m_defaultCacheBuilt = true;
    }

    if (m_lastCachedCustomer != rule.customerName) {
        m_cachedCustomRules.clear();
        for (const PriceRule &pr : rule.customPriceRules) {
            QString key = ProvinceUtils::normalize(pr.province);
            if (!key.isEmpty())
                m_cachedCustomRules.insert(key, pr);
        }
        m_lastCachedCustomer = rule.customerName;
    }
}

double FreightCalculator::calculateFreight(const OrderData &order, const CustomerRule &rule)
{
    QString unused;
    return calculateFreightDetail(order, rule, unused);
}

double FreightCalculator::calculateFreightDetail(const OrderData &order, const CustomerRule &rule, QString &outRuleDesc)
{
    outRuleDesc.clear();

    // ★ 标准化省份，确保 O(1) 哈希命中
    QString province = ProvinceUtils::standardize(order.destinationProvince);

    CalculationRule::Mode mode = CalculationRule::modeFromString(rule.calculationMode);
    double effectiveWeight = CalculationRule::getEffectiveWeight(order.actualWeight, order.volumetricWeight, mode);

    if (effectiveWeight <= 0 && m_globalRules.noWeightDefaultPrice > 0) {
        double result = m_globalRules.noWeightDefaultPrice;
        result = CalculationRule::applyPriceIncreases(result, 0, m_globalRules.activityRules, order.businessTime);
        result = CalculationRule::applyPriceIncreases(result, 0, m_globalRules.tempPriceIncreases, order.businessTime);
        // 省份加价 — 无重量时也要匹配（PerKg 无效）
        for (const PriceIncreaseRule &ppi : m_globalRules.provincePriceIncreases) {
            if (!ppi.isActive) continue;
            if (CalculationRule::provinceMatches(province, ppi.province)) {
                if (ppi.mode != IncreaseMode::PerKg)  // 无重量时 PerKg 无效
                    result = getIncreaseFunc(ppi.mode)(result, 0, ppi.amount);
            }
        }
        result = std::round(result * 100.0) / 100.0;
        outRuleDesc = QStringLiteral("无重量默认价");
        return result;
    }

    if (effectiveWeight <= 0) {
        return 0.0;
    }

    PriceRule priceRule = findPriceRule(province, rule);

    CalculationRule::Mode calcMode = CalculationRule::modeFromString(rule.calculationMode);

    // 判断是否为客户自定义规则
    bool isCustomRule = false;
    for (const PriceRule &pr : rule.customPriceRules) {
        if (pr.province == priceRule.province && pr.region == priceRule.region) {
            isCustomRule = true;
            break;
        }
    }

    double freight = 0.0;
    bool matched = false;
    auto calcFunc = CalculationRule::getCalcFunc(calcMode);
    for (const WeightSegment &seg : priceRule.segments) {
        if (CalculationRule::isWeightInRange(effectiveWeight, seg)) {
            freight = calcFunc(effectiveWeight, priceRule.segments);
            matched = true;
            break;
        }
    }

    if (!matched) {
        outRuleDesc = QStringLiteral("未匹配到重量段(重量=%1kg)").arg(effectiveWeight, 0, 'f', 2);
        return 0.0;
    }

    // 构建规则描述
    QString ruleDesc;
    if (isCustomRule && !rule.customerName.isEmpty()) {
        ruleDesc = rule.customerName + QStringLiteral("-");
    } else {
        ruleDesc = QStringLiteral("默认报价-");
    }
    ruleDesc += priceRule.province;
    if (!priceRule.region.isEmpty())
        ruleDesc += QStringLiteral("(%1)").arg(priceRule.region);

    // ★ 辅助：生成加价描述 "名称(+金额/百分比/KG)"
    auto increaseDesc = [](const PriceIncreaseRule &r) -> QString {
        QString base = r.name.isEmpty() ? QStringLiteral("加价") : r.name;
        switch (r.mode) {
        case IncreaseMode::PerTicketFixed:   return QStringLiteral("%1(+%2元)").arg(base).arg(r.amount, 0, 'f', 1);
        case IncreaseMode::PerTicketPercent: return QStringLiteral("%1(+%2%)").arg(base).arg(r.amount * 100, 0, 'f', 0);
        case IncreaseMode::PerKg:            return QStringLiteral("%1(+%2元/kg)").arg(base).arg(r.amount, 0, 'f', 1);
        }
        return base;
    };

    // 活动加价（只比较日期）
    for (const PriceIncreaseRule &ar : m_globalRules.activityRules) {
        if (ar.isDateInRange(order.businessTime))
            ruleDesc += increaseDesc(ar);
    }
    freight = CalculationRule::applyPriceIncreases(freight, effectiveWeight, m_globalRules.activityRules, order.businessTime);

    // 临时加价
    for (const PriceIncreaseRule &tpi : m_globalRules.tempPriceIncreases) {
        if (tpi.isDateInRange(order.businessTime))
            ruleDesc += increaseDesc(tpi);
    }
    freight = CalculationRule::applyPriceIncreases(freight, effectiveWeight, m_globalRules.tempPriceIncreases, order.businessTime);

    // 省份加价
    for (const PriceIncreaseRule &ppi : m_globalRules.provincePriceIncreases) {
        if (!ppi.isActive) continue;
        if (CalculationRule::provinceMatches(province, ppi.province)) {
            ruleDesc += increaseDesc(ppi);
            freight = getIncreaseFunc(ppi.mode)(freight, effectiveWeight, ppi.amount);
        }
    }

    // 拉均重加价
    if (m_globalRules.runtimeAvgSurcharge > 0 && calcMode == CalculationRule::Mode::AverageWeight) {
        freight += m_globalRules.runtimeAvgSurcharge;
        ruleDesc += QStringLiteral("+拉均重加价%1").arg(m_globalRules.runtimeAvgSurcharge, 0, 'f', 2);
    }

    freight = std::round(freight * 100.0) / 100.0;

    outRuleDesc = ruleDesc;
    return freight;
}

void FreightCalculator::calculateBatch(QList<OrderData> &orders, const CustomerRule &rule, int threadCount)
{
    if (orders.isEmpty())
        return;

    int total = orders.size();
    QAtomicInt progress(0);
    QAtomicInt errorCount(0);

    buildRuleCache(rule);

    int effectiveThreads = qMax(1, qMin(threadCount, QThread::idealThreadCount()));
    int chunkSize = qMax(1000, total / effectiveThreads);
    int batches = (total + chunkSize - 1) / chunkSize;

    QVector<QFuture<void>> futures;
    futures.reserve(batches);

    OrderData *dataPtr = orders.data();

    for (int i = 0; i < batches; ++i) {
        int start = i * chunkSize;
        int end = qMin(start + chunkSize, total);

        futures.append(QtConcurrent::run([&, start, end]() {
            for (int idx = start; idx < end; ++idx) {
                OrderData &order = dataPtr[idx];
                QString ruleDesc;
                double freight = calculateFreightDetail(order, rule, ruleDesc);
                order.freight = freight;
                order.usedRule = ruleDesc;
                if (freight <= 0 && order.actualWeight > 0) {
                    order.isValid = false;
                    order.errorMessage = QStringLiteral("计算结果为0，可能未匹配到价格规则");
                    errorCount.fetchAndAddRelaxed(1);
                } else {
                    order.isValid = true;
                }

                int done = progress.fetchAndAddRelaxed(1) + 1;
                if (done % 2000 == 0 || done == total) {
                    int pct = static_cast<int>(done * 100.0 / total);
                    emit progressUpdated(qMin(99, pct));
                }
            }
        }));
    }

    for (auto &f : futures)
        f.waitForFinished();

    emit progressUpdated(100);
    emit calculationComplete(total, errorCount.loadAcquire());
}

void FreightCalculator::setDefaultPriceTable(const QList<PriceRule> &rules)
{
    m_defaultPriceTable = rules;
    m_defaultCacheBuilt = false;
}

QList<PriceRule> FreightCalculator::defaultPriceTable() const
{
    return m_defaultPriceTable;
}

void FreightCalculator::setGlobalRules(const GlobalRules &rules)
{
    m_globalRules = rules;
}

GlobalRules FreightCalculator::globalRules() const
{
    return m_globalRules;
}

double FreightCalculator::quickCalculate(double weight, const QString &province, const CustomerRule &rule, const QDate &time)
{
    OrderData tempOrder;
    tempOrder.actualWeight = weight;
    tempOrder.volumetricWeight = weight;
    tempOrder.destinationProvince = province;
    tempOrder.businessTime = time.isValid() ? time : QDate::currentDate();
    tempOrder.isValid = true;

    return calculateFreight(tempOrder, rule);
}

PriceRule FreightCalculator::findPriceRule(const QString &province, const CustomerRule &rule)
{
    QString normalized = ProvinceUtils::normalize(province);

    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_lastCachedCustomer == rule.customerName && !m_cachedCustomRules.isEmpty()) {
            auto it = m_cachedCustomRules.constFind(normalized);
            if (it != m_cachedCustomRules.constEnd())
                return it.value();
        }
    }

    if (!rule.useDefaultPrice || !rule.customPriceRules.isEmpty()) {
        for (const PriceRule &pr : rule.customPriceRules) {
            QString ruleProvince = ProvinceUtils::normalize(pr.province);
            if (ruleProvince == normalized) {
                return pr;
            }
        }
        for (const PriceRule &pr : rule.customPriceRules) {
            QString ruleProvince = ProvinceUtils::normalize(pr.province);
            if (normalized.contains(ruleProvince) || ruleProvince.contains(normalized)) {
                return pr;
            }
        }
        for (const PriceRule &pr : rule.customPriceRules) {
            QString ruleProvince = ProvinceUtils::normalize(pr.province);
            if (fuzzyProvinceMatch(normalized, ruleProvince)) {
                return pr;
            }
        }
    }

    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_defaultCacheBuilt && !m_cachedDefaultRules.isEmpty()) {
            auto it = m_cachedDefaultRules.constFind(normalized);
            if (it != m_cachedDefaultRules.constEnd())
                return it.value();
        }
    }

    for (const PriceRule &pr : m_defaultPriceTable) {
        QString ruleProvince = ProvinceUtils::normalize(pr.province);
        if (ruleProvince == normalized) {
            return pr;
        }
    }
    for (const PriceRule &pr : m_defaultPriceTable) {
        QString ruleProvince = ProvinceUtils::normalize(pr.province);
        if (normalized.contains(ruleProvince) || ruleProvince.contains(normalized)) {
            return pr;
        }
    }
    for (const PriceRule &pr : m_defaultPriceTable) {
        QString ruleProvince = ProvinceUtils::normalize(pr.province);
        if (fuzzyProvinceMatch(normalized, ruleProvince)) {
            return pr;
        }
    }

    return PriceRule();
}
