#include "MappingTemplate.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// ==================== MappingTemplate ====================

QJsonObject MappingTemplate::toJson() const
{
    QJsonObject obj;
    obj["name"] = name;
    obj["courier"] = courier;
    QJsonObject mapObj;
    for (auto it = headerMapping.constBegin(); it != headerMapping.constEnd(); ++it)
        mapObj[it.key()] = it.value();
    obj["headerMapping"] = mapObj;
    return obj;
}

MappingTemplate MappingTemplate::fromJson(const QJsonObject &obj)
{
    MappingTemplate tpl;
    tpl.name = obj["name"].toString();
    tpl.courier = obj["courier"].toString();
    QJsonObject mapObj = obj["headerMapping"].toObject();
    for (auto it = mapObj.constBegin(); it != mapObj.constEnd(); ++it)
        tpl.headerMapping[it.key()] = it.value().toString();
    return tpl;
}

QMap<QString, int> MappingTemplate::applyTo(const QStringList &actualHeaders) const
{
    QMap<QString, int> result;
    for (auto tplIt = headerMapping.constBegin(); tplIt != headerMapping.constEnd(); ++tplIt) {
        const QString &fieldKey = tplIt.key();
        const QString &expectedName = tplIt.value();

        // 1. 优先精确匹配
        for (int i = 0; i < actualHeaders.size(); ++i) {
            if (actualHeaders[i].trimmed() == expectedName) {
                result[fieldKey] = i;
                goto nextField;
            }
        }
        // 2. 包含匹配（模糊）
        for (int i = 0; i < actualHeaders.size(); ++i) {
            if (actualHeaders[i].contains(expectedName, Qt::CaseInsensitive)) {
                result[fieldKey] = i;
                goto nextField;
            }
        }
        nextField:;
    }
    return result;
}

// ==================== MappingTemplateManager ====================

MappingTemplateManager::MappingTemplateManager()
{
    load();
    ensureBuiltinsExist();
}

QString MappingTemplateManager::filePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dir.isEmpty()) {
        QDir().mkpath(dir);
        return dir + QDir::separator() + "column_mappings.json";
    }
    return "column_mappings.json";
}

void MappingTemplateManager::load()
{
    QFile f(filePath());
    if (!f.open(QIODevice::ReadOnly))
        return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return;

    m_templates.clear();
    for (const QJsonValue &v : doc.array()) {
        if (v.isObject())
            m_templates.append(MappingTemplate::fromJson(v.toObject()));
    }
}

void MappingTemplateManager::persist() const
{
    QJsonArray arr;
    for (const MappingTemplate &tpl : m_templates)
        arr.append(tpl.toJson());

    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        f.close();
    }
}

QList<MappingTemplate> MappingTemplateManager::allTemplates() const
{
    return m_templates;
}

QStringList MappingTemplateManager::templateNames() const
{
    QStringList names;
    for (const MappingTemplate &tpl : m_templates)
        names.append(tpl.name);
    return names;
}

MappingTemplate MappingTemplateManager::get(const QString &name) const
{
    int idx = indexOf(name);
    if (idx >= 0)
        return m_templates[idx];
    return {};
}

int MappingTemplateManager::indexOf(const QString &name) const
{
    for (int i = 0; i < m_templates.size(); ++i) {
        if (m_templates[i].name == name)
            return i;
    }
    return -1;
}

void MappingTemplateManager::save(const MappingTemplate &tpl)
{
    int idx = indexOf(tpl.name);
    if (idx >= 0) {
        m_templates[idx] = tpl;
    } else {
        m_templates.append(tpl);
    }
    persist();
}

void MappingTemplateManager::remove(const QString &name)
{
    int idx = indexOf(name);
    if (idx >= 0) {
        m_templates.removeAt(idx);
        persist();
    }
}

QString MappingTemplateManager::defaultTemplateName(const QString &courier) const
{
    // 查找该快递公司的默认模板
    for (const MappingTemplate &tpl : m_templates) {
        if (tpl.courier == courier && tpl.name.contains(QStringLiteral("默认")))
            return tpl.name;
    }
    return {};
}

void MappingTemplateManager::ensureBuiltinsExist()
{
    QList<MappingTemplate> builtins = builtinTemplates();
    bool changed = false;
    for (const MappingTemplate &tpl : builtins) {
        if (indexOf(tpl.name) < 0) {
            m_templates.append(tpl);
            changed = true;
        }
    }
    if (changed)
        persist();
}

// ==================== 预置模板 ====================

QList<MappingTemplate> MappingTemplateManager::builtinTemplates()
{
    // 各字段用最常用的 Excel 列名作为默认映射
    auto makeDefault = [](const QString &courier,
                          const QString &waybill,
                          const QString &time,
                          const QString &orderCustomer,
                          const QString &billingCustomer,
                          const QString &destProv,
                          const QString &weight,
                          const QString &volWeight,
                          const QString &freight) -> MappingTemplate {
        MappingTemplate tpl;
        tpl.name = courier + QStringLiteral("默认");
        tpl.courier = courier;
        // key: 内部字段  value: Excel 中期望的列名
        tpl.headerMapping["运单号"]   = waybill;
        tpl.headerMapping["业务时间"] = time;
        tpl.headerMapping["订单客户"] = orderCustomer;
        tpl.headerMapping["客户"]     = billingCustomer;
        tpl.headerMapping["目的省份"] = destProv;
        tpl.headerMapping["实际重量"] = weight;
        tpl.headerMapping["体积重"]   = volWeight;
        tpl.headerMapping["运费"]     = freight;
        tpl.headerMapping["使用的规则"] = QStringLiteral("使用的规则");
        tpl.headerMapping["错误信息"] = QStringLiteral("错误信息");
        return tpl;
    };

    return {
        // 申通 — 业内最通用的列名
        makeDefault(QStringLiteral("申通"),
            QStringLiteral("运单号"), QStringLiteral("业务时间"),
            QStringLiteral("订单客户"), QStringLiteral("结算对象"),
            QStringLiteral("目的省份"), QStringLiteral("结算重量"),
            QStringLiteral("体积重"), QStringLiteral("运费")),

        // 中通 — 类似申通，客户列习惯用"结算客户"
        makeDefault(QStringLiteral("中通"),
            QStringLiteral("运单号"), QStringLiteral("业务时间"),
            QStringLiteral("订单客户"), QStringLiteral("结算客户"),
            QStringLiteral("目的省份"), QStringLiteral("计费重量"),
            QStringLiteral("体积重"), QStringLiteral("运费")),

        // 圆通 — 习惯用"目的地"
        makeDefault(QStringLiteral("圆通"),
            QStringLiteral("运单号"), QStringLiteral("业务时间"),
            QStringLiteral("订单客户"), QStringLiteral("结算对象"),
            QStringLiteral("目的地"), QStringLiteral("实际重量"),
            QStringLiteral("体积重"), QStringLiteral("运费")),

        // 韵达 — 习惯用"收货省"和"实重"
        makeDefault(QStringLiteral("韵达"),
            QStringLiteral("运单号"), QStringLiteral("业务时间"),
            QStringLiteral("订单客户"), QStringLiteral("结算对象"),
            QStringLiteral("收货省"), QStringLiteral("实重"),
            QStringLiteral("体积重"), QStringLiteral("运费")),

        // 顺丰 — 大客户体系，列名较规范
        makeDefault(QStringLiteral("顺丰"),
            QStringLiteral("运单编号"), QStringLiteral("发货时间"),
            QStringLiteral("寄件客户"), QStringLiteral("结算对象"),
            QStringLiteral("收件省份"), QStringLiteral("计费重量"),
            QStringLiteral("体积重量"), QStringLiteral("应收运费")),

        // 京东物流 — 类似顺丰，偏仓储风格
        makeDefault(QStringLiteral("京东"),
            QStringLiteral("运单号"), QStringLiteral("业务时间"),
            QStringLiteral("订单客户"), QStringLiteral("结算客户"),
            QStringLiteral("目的省份"), QStringLiteral("结算重量"),
            QStringLiteral("体积重"), QStringLiteral("运费")),
    };
}
