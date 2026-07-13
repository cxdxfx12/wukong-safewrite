#ifndef PRICETABLE_H
#define PRICETABLE_H

#include <QString>
#include <QList>
#include <QMap>
#include <QDateTime>

// 重量段定义
struct WeightSegment {
    double minWeight = 0;           // 最小重量 (kg)
    double maxWeight = -1;          // 最大重量 (kg), -1 表示无上限
    double firstWeightPrice = 0;    // 首重价格 (元)
    double additionalPrice = 0;     // 续重价格 (元/kg)
    bool isFullAdditional = false;  // 是否全续模式
    bool isHundredGram = false;     // 是否百克续
};

// 价格规则（按区域/省份）
struct PriceRule {
    QString region;                 // 区域名称 (一区、二区...)
    QString province;               // 省份
    QList<WeightSegment> segments;  // 重量段列表
};

// ===================================================================
// 加价模式枚举 — 三种计费方式
// ===================================================================
enum class IncreaseMode {
    PerTicketFixed,   // 单票固定加价：每票 +X 元（不受重量影响）
    PerTicketPercent, // 单票加百分比：运费 × (1 + X%)，如 0.1 = +10%
    PerKg             // 每KG加价：运费 + 重量(kg) × X元
};

inline QString increaseModeLabel(IncreaseMode mode) {
    switch (mode) {
    case IncreaseMode::PerTicketFixed:   return QStringLiteral("单票固定加价");
    case IncreaseMode::PerTicketPercent: return QStringLiteral("单票加百分比");
    case IncreaseMode::PerKg:            return QStringLiteral("每KG加价");
    }
    return {};
}

// ★ 策略分派：加价模式 → 计算函数（消除所有 switch/case）
using IncreaseFunc = double (*)(double freight, double weight, double amount);

inline double incFixedPerTicket(double freight, double, double amount)    { return freight + amount; }
inline double incPercentPerTicket(double freight, double, double amount)  { return freight * (1.0 + amount); }
inline double incPerKg(double freight, double weight, double amount)      { return freight + weight * amount; }

inline IncreaseFunc getIncreaseFunc(IncreaseMode mode) {
    static const IncreaseFunc table[] = { incFixedPerTicket, incPercentPerTicket, incPerKg };
    return table[static_cast<int>(mode)];
}

// ===================================================================
// 统一加价规则 — 活动/临时/省份三种规则共用此结构
// ===================================================================
struct PriceIncreaseRule {
    QString name;                   // 规则名称
    IncreaseMode mode = IncreaseMode::PerTicketFixed;  // 加价模式
    double amount = 0.0;           // 加价数值（固定=元，百分比=小数如0.1即10%，每KG=元/kg）
    QDateTime startTime;           // 生效开始（活动/临时规则使用）
    QDateTime endTime;             // 生效结束（活动/临时规则使用）
    QString province;              // 目标省份（省份加价规则使用）
    bool isActive = true;          // 是否启用

    /// 判断日期是否在规则生效范围内（活动/临时规则用，只比较年月日）
    bool isDateInRange(const QDate &date) const {
        if (!isActive || !date.isValid()) return false;
        if (!startTime.isValid() || !endTime.isValid()) return false;
        return date >= startTime.date() && date <= endTime.date();
    }
};

// ===================================================================
// 全局规则（对所有客户生效）
// ===================================================================
struct GlobalRules {
    double noWeightDefaultPrice = 0;                          // 无重量默认价格 (元)
    QList<PriceIncreaseRule> activityRules;                   // 活动加价规则列表
    QList<PriceIncreaseRule> tempPriceIncreases;              // 临时加价规则列表
    QList<PriceIncreaseRule> provincePriceIncreases;          // 按省份加价规则列表

    // 拉均重模式加价配置
    double avgWeightBase = 0.5;          // 基准重量(kg)
    double avgWeightUpperLimit = 1.0;    // 上限重量(kg)
    double avgWeightIncrement = 0.1;     // 递增步长(kg)
    double avgWeightSurcharge = 0.1;     // 每步长加价金额(元)

    // 运行时计算的每包裹加价（非持久化）
    double runtimeAvgSurcharge = 0.0;
};

// 客户规则（每客户单独配置）
struct CustomerRule {
    QString customerName;           // 客户名称
    QString mappingHeader;          // 映射的表头名称
    QString calculationMode;        // 计算模式: "实际重量"|"体积重"|"拉均重"
    bool useDefaultPrice = true;    // 是否使用默认报价
    QList<PriceRule> customPriceRules;  // 自定义报价规则
    QString courier;                // 关联的快递公司
};

// 区域-省份映射（默认报价表用）
struct RegionMapping {
    QString region;
    QStringList provinces;
};

#endif // PRICETABLE_H
