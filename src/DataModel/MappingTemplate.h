#ifndef MAPPINGTEMPLATE_H
#define MAPPINGTEMPLATE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QJsonObject>

// 单个映射模板
struct MappingTemplate {
    QString name;       // 模板名称，如 "申通标准格式"
    QString courier;    // 关联快递公司，如 "申通"
    // 内部字段名 → 期望的 Excel 列名（精确匹配优先，其次关键词匹配）
    QMap<QString, QString> headerMapping;

    QJsonObject toJson() const;
    static MappingTemplate fromJson(const QJsonObject &obj);

    /// 用此模板对实际表头做匹配，返回 内部字段名→列索引
    QMap<QString, int> applyTo(const QStringList &actualHeaders) const;
};

// 模板管理器（持久化到 AppData/column_mappings.json）
class MappingTemplateManager
{
public:
    MappingTemplateManager();

    QList<MappingTemplate> allTemplates() const;
    QStringList templateNames() const;

    MappingTemplate get(const QString &name) const;
    void save(const MappingTemplate &tpl);
    void remove(const QString &name);

    // 获取快递公司对应的默认模板名
    QString defaultTemplateName(const QString &courier) const;

    // 预置模板
    static QList<MappingTemplate> builtinTemplates();

private:
    QList<MappingTemplate> m_templates;

    static QString filePath();
    void load();
    void persist() const;

    void ensureBuiltinsExist();
    int indexOf(const QString &name) const;
};

#endif // MAPPINGTEMPLATE_H
