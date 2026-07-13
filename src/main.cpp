#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include "MainWindow.h"
#include "Utils/Logger.h"
#include "Utils/ConfigManager.h"
#include "Utils/ThemeManager.h"
#include "Auth/LoginDialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 应用信息
    QApplication::setApplicationName("悟空运费结算");
    QApplication::setApplicationDisplayName("悟空运费结算");
    QApplication::setApplicationVersion("1.2.3");
    QApplication::setOrganizationName("杭州喵喵至家网络有限公司");

    // 设置应用程序图标
    QApplication::setWindowIcon(QIcon(":/app_icon.png"));

    // 加载主题样式表（用户可在全局规则页切换）
    QString savedTheme = ThemeManager::currentTheme();
    app.setStyleSheet(ThemeManager::themeQSS(savedTheme));

    // 初始化日志
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!logDir.isEmpty()) {
        QDir().mkpath(logDir);
        Logger::instance().setLogFile(logDir + QDir::separator() + QStringLiteral("freight_calculator.log"));
    } else {
        Logger::instance().setLogFile(QStringLiteral("freight_calculator.log"));
    }
    Logger::instance().info("Application started");

    // 显示授权登录对话框
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted || !loginDlg.isAuthorized()) {
        Logger::instance().info("Authorization failed or cancelled");
        return 0;
    }

    Logger::instance().info("Authorization passed");

    // 创建主窗口
    MainWindow window;
    window.setWindowTitle("悟空运费结算");
    window.resize(1280, 800);
    window.show();

    int ret = app.exec();

    Logger::instance().info("Application exited");
    return ret;
}
