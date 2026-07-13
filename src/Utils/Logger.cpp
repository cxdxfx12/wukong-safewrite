#include "Logger.h"

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
}

Logger::~Logger()
{
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
}

void Logger::setLogFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_stream.flush();
        m_file.close();
    }
    m_filePath = filePath;
    m_file.setFileName(filePath);
    if (m_file.open(QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_file);
    }
}

void Logger::log(Level level, const QString &message)
{
    QMutexLocker locker(&m_mutex);
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelToString(level), message);
    m_stream << formatted << "\n";
    m_stream.flush();
}

void Logger::debug(const QString &msg)
{
    log(Level::Debug, msg);
}

void Logger::info(const QString &msg)
{
    log(Level::Info, msg);
}

void Logger::warning(const QString &msg)
{
    log(Level::Warning, msg);
}

void Logger::error(const QString &msg)
{
    log(Level::Error, msg);
}

QString Logger::levelToString(Level level)
{
    switch (level) {
    case Level::Debug:   return "DEBUG";
    case Level::Info:    return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error:   return "ERROR";
    default:      return "UNKNOWN";
    }
}
