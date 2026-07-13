#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QThread>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_settings("WukongFreight", "WukongFreight")
{
}

int ConfigManager::defaultThreadCount() const
{
    return m_settings.value("defaultThreadCount", 0).toInt();
}

void ConfigManager::setDefaultThreadCount(int count)
{
    m_settings.setValue("defaultThreadCount", qBound(1, count, QThread::idealThreadCount()));
    m_settings.sync();
}

int ConfigManager::batchSize() const
{
    return m_settings.value("batchSize", 10000).toInt();
}

void ConfigManager::setBatchSize(int size)
{
    m_settings.setValue("batchSize", qMax(100, size));
    m_settings.sync();
}

QString ConfigManager::lastImportPath() const
{
    return m_settings.value("lastImportPath", "").toString();
}

void ConfigManager::setLastImportPath(const QString &path)
{
    m_settings.setValue("lastImportPath", path);
    m_settings.sync();
}

QString ConfigManager::lastExportPath() const
{
    return m_settings.value("lastExportPath", "").toString();
}

void ConfigManager::setLastExportPath(const QString &path)
{
    m_settings.setValue("lastExportPath", path);
    m_settings.sync();
}

QString ConfigManager::rulesFilePath() const
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (defaultPath.isEmpty()) {
        defaultPath = QDir::homePath() + QStringLiteral("/.WukongFreight");
    }
    defaultPath += QStringLiteral("/rules.json");
    return m_settings.value("rulesFilePath", defaultPath).toString();
}

void ConfigManager::setRulesFilePath(const QString &path)
{
    m_settings.setValue("rulesFilePath", path);
    m_settings.sync();
}

void ConfigManager::sync()
{
    m_settings.sync();
}
