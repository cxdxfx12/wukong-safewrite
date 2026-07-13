#include "ProvinceUtils.h"
#include <QHash>

const QStringList &ProvinceUtils::getSuffixes()
{
    static const QStringList suffixes = {
        QStringLiteral("省"), QStringLiteral("市"),
        QStringLiteral("自治区"), QStringLiteral("特别行政区")
    };
    return suffixes;
}

QString ProvinceUtils::normalize(const QString &province)
{
    QString s = province.trimmed();
    const QStringList &suffixes = getSuffixes();
    for (const QString &sf : suffixes) {
        if (s.endsWith(sf)) {
            s.chop(sf.size());
            break;
        }
    }
    return s.trimmed();
}

QString ProvinceUtils::extractTwoCharPrefix(const QString &province)
{
    QString normalized = normalize(province);
    if (normalized.size() >= 2) {
        return normalized.left(2);
    }
    return normalized;
}

QString ProvinceUtils::standardize(const QString &province)
{
    // 第一步：去后缀
    QString s = normalize(province);
    if (s.isEmpty()) return province;

    // 第二步：别名 → 标准简称（报价表使用的名称）
    static const QHash<QString, QString> alias = {
        // 全称 → 简称
        {QStringLiteral("新疆维吾尔"), QStringLiteral("新疆")},
        {QStringLiteral("西藏"),      QStringLiteral("西藏")},
        {QStringLiteral("广西壮族"),  QStringLiteral("广西")},
        {QStringLiteral("宁夏回族"),  QStringLiteral("宁夏")},
        {QStringLiteral("内蒙古"),   QStringLiteral("内蒙古")},
        {QStringLiteral("黑龙江"),   QStringLiteral("黑龙江")},
        // 直辖市（去"市"后只剩2字或更少，本身就是简称）
        // 北京/上海/天津/重庆 normalize后已是简称
        // 常见别名
        {QStringLiteral("内蒙古自治区"), QStringLiteral("内蒙古")},
    };

    auto it = alias.constFind(s);
    return it != alias.constEnd() ? it.value() : s;
}

bool ProvinceUtils::matches(const QString &input, const QString &target)
{
    QString normInput = normalize(input);
    QString normTarget = normalize(target);

    if (normInput.isEmpty() || normTarget.isEmpty()) {
        return false;
    }

    if (normInput == normTarget) {
        return true;
    }

    if (normInput.contains(normTarget) || normTarget.contains(normInput)) {
        return true;
    }

    if (normInput.size() >= 2 && normTarget.size() >= 2 &&
        normInput.left(2) == normTarget.left(2)) {
        return true;
    }

    return false;
}
