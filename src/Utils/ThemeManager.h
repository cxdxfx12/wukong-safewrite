#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QStringList>

class ThemeManager {
public:
    static QStringList themeNames();
    static QString defaultTheme();   // 首次使用的默认
    static QString themeQSS(const QString &name);
    static QString currentTheme();
    static void setCurrentTheme(const QString &name);

private:
    static QString themeFilePath();
};

#endif
