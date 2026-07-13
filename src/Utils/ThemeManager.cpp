#include "ThemeManager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>

static const QString THEME_KEY = QStringLiteral("ui_theme");

QString ThemeManager::themeFilePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QDir::separator() + QStringLiteral("theme.txt");
}

QStringList ThemeManager::themeNames()
{
    return {QStringLiteral("活力橙（默认）"), QStringLiteral("macOS 风格"), QStringLiteral("暗夜模式"), QStringLiteral("渐变紫")};
}

QString ThemeManager::defaultTheme()
{
    return QStringLiteral("活力橙（默认）");
}

QString ThemeManager::currentTheme()
{
    QFile f(themeFilePath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString t = f.readAll().trimmed();
        f.close();
        if (themeNames().contains(t)) return t;
    }
    return defaultTheme();
}

void ThemeManager::setCurrentTheme(const QString &name)
{
    QFile f(themeFilePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(name.toUtf8());
        f.close();
    }
}

QString ThemeManager::themeQSS(const QString &name)
{
    // ========== 共用基础（所有主题都有的） ==========
    auto base = []() -> QString {
        return QStringLiteral(R"(
            QWidget { font-family: "Microsoft YaHei UI"; font-size: 11px; }
            * { outline: none; }
            QSplitter::handle { width: 3px; }
            QSplitter::handle:hover { background-color: #888; }
            QSpinBox, QDoubleSpinBox { padding: 3px 24px 3px 8px; min-height: 26px; border-radius: 4px; }
            QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-position: top right; subcontrol-origin: margin; }
            QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-position: bottom right; subcontrol-origin: margin; }
            QLabel { background-color: transparent; }
            QCheckBox { background-color: transparent; spacing: 6px; }
        )");
    };

    // ========== 活力橙 ==========
    if (name == QStringLiteral("活力橙（默认）")) {
        return base() + QStringLiteral(R"(
            QMainWindow, QDialog { background-color: #f5f6fa; }
            QWidget { color: #2c3e50; }
            QGroupBox { background: #fff; border:1px solid #e8ecf1; border-radius:10px; margin-top:14px; padding-top:12px; font-weight:bold; color:#2c3e50; }
            QGroupBox::title { left:12px; padding:0 6px; color:#2c3e50; }
            QTableView { background:#fff; border:1px solid #e8ecf1; border-radius:8px; gridline-color:#f0f3f7; alternate-background-color:#fafcfe; }
            QTableView::item:selected { background:#fff0e0; color:#e65c00; }
            QHeaderView::section { background:#f5f7fa; color:#5a6c7d; padding:8px 10px; border:none; border-bottom:2px solid #ffb347; font-weight:bold; font-size:11px; }
            QComboBox { background:#fff; border:1px solid #dde2ea; border-radius:6px; padding:4px 10px; min-height:28px; color:#34495e; }
            QComboBox:hover { border-color:#ffb347; }
            QComboBox::drop-down { border:none; width:20px; }
            QComboBox::down-arrow { border-left:4px solid transparent; border-right:4px solid transparent; border-top:5px solid #7f8c9b; margin-right:6px; }
            QComboBox QAbstractItemView { background:#fff; border:1px solid #e8ecf1; border-radius:6px; selection-background-color:#fff5e8; selection-color:#ff6a00; }
            QLineEdit { background:#fff; border:1px solid #dde2ea; border-radius:6px; padding:4px 10px; min-height:28px; color:#34495e; }
            QLineEdit:hover { border-color:#ffb347; }
            QLineEdit:focus { border-color:#ff7f20; }
            QPushButton { background:#fff; border:1px solid #dde2ea; border-radius:6px; padding:4px 14px; min-height:28px; color:#34495e; }
            QPushButton:hover { background:#f5f7fa; border-color:#ff9f43; color:#ff7f20; }
            QPushButton:pressed { background:#eef2f7; border-color:#ff7f20; }
            QPushButton:disabled { background:#f0f2f5; color:#b0b8c4; border-color:#e4e8ed; }
            QPushButton#primaryButton { background:#ff8c32; color:#fff; border:1px solid #ff7f20; font-weight:bold; }
            QPushButton#primaryButton:hover { background:#ff7a18; border-color:#ff6a00; }
            QPushButton#secondaryButton { background:#fff; border:1px solid #dde2ea; color:#34495e; }
            QPushButton#secondaryButton:hover { background:#f5f7fa; border-color:#ff9f43; color:#ff7f20; }
            QPushButton#smallButton { background:#fff; border:1px solid #dde2ea; color:#34495e; padding:2px 10px; min-height:24px; }
            QPushButton#smallButton:hover { background:#f5f7fa; border-color:#ff9f43; color:#ff7f20; }
            QPushButton#dangerButton { background:#fff; border:1px solid #ff6b6b; color:#ff4757; }
            QPushButton#dangerButton:hover { background:#fff0f0; border-color:#ff4757; }
            QProgressBar { background:#eef2f7; border:none; border-radius:6px; text-align:center; color:#5a6c7d; font-weight:bold; height:20px; }
            QProgressBar::chunk { background:#ff8c32; border-radius:6px; }
            QStatusBar { background:#fff; border-top:1px solid #e8ecf1; color:#7f8c9b; }
            QScrollBar:vertical { background:#f5f7fa; width:8px; border-radius:4px; }
            QScrollBar::handle:vertical { background:#c8d0db; border-radius:4px; min-height:24px; }
            QScrollBar::handle:vertical:hover { background:#ff9f43; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
            QScrollBar:horizontal { background:#f5f7fa; height:8px; border-radius:4px; }
            QScrollBar::handle:horizontal { background:#c8d0db; border-radius:4px; min-width:24px; }
            QScrollBar::handle:horizontal:hover { background:#ff9f43; }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
            QTabWidget::pane { border:1px solid #d0d0d0; border-radius:6px; background:#fff; }
            QTabBar::tab { background:#f5f6fa; border:1px solid #d0d0d0; padding:6px 18px; margin-right:2px; border-top-left-radius:6px; border-top-right-radius:6px; color:#666; }
            QTabBar::tab:selected { background:#fff; color:#ff6a00; font-weight:bold; border-bottom:2px solid #ff7f20; }
            QTabBar::tab:hover { background:#e8ecf5; }
            QListWidget { background:#fff; border:1px solid #e8ecf1; border-radius:8px; padding:4px; }
            QListWidget::item:selected { background:#ff8c32; color:#fff; font-weight:bold; }
            QListWidget::item:hover:!selected { background:#fff5e8; color:#ff6a00; }
            QMenuBar { background:#fff; border-bottom:1px solid #e8ecf1; }
            QMenu { background:#fff; border:1px solid #e8ecf1; border-radius:8px; padding:4px; }
            QMenu::item:selected { background:#fff5e8; color:#ff6a00; }
        )");
    }

    // ========== macOS 风格 ==========
    if (name == QStringLiteral("macOS 风格")) {
        return base() + QStringLiteral(R"(
            QMainWindow, QDialog { background-color: #f5f5f7; }
            QWidget { color: #1d1d1f; font-size: 12px; }
            QGroupBox { background: #fff; border:1px solid #e5e5ea; border-radius:12px; margin-top:14px; padding-top:14px; font-weight:bold; color:#1d1d1f; }
            QGroupBox::title { left:14px; padding:0 6px; color:#1d1d1f; font-size:12px; }
            QTableView { background:#fff; border:1px solid #e5e5ea; border-radius:10px; gridline-color:#f0f0f5; alternate-background-color:#fafafc; }
            QTableView::item:selected { background:#e8f0fe; color:#007aff; }
            QHeaderView::section { background:#f9f9fb; color:#6e6e73; padding:8px 12px; border:none; border-bottom:1px solid #e5e5ea; font-weight:600; font-size:11px; }
            QComboBox { background:#f2f2f7; border:1px solid #e5e5ea; border-radius:8px; padding:4px 12px; min-height:28px; color:#1d1d1f; }
            QComboBox:hover { background:#e8e8ed; }
            QComboBox:focus { border-color:#007aff; background:#fff; }
            QComboBox::drop-down { border:none; width:22px; }
            QComboBox::down-arrow { border-left:4px solid transparent; border-right:4px solid transparent; border-top:5px solid #8e8e93; margin-right:8px; }
            QComboBox QAbstractItemView { background:#fff; border:1px solid #e5e5ea; border-radius:8px; selection-background-color:#007aff; selection-color:#fff; }
            QLineEdit { background:#f2f2f7; border:1px solid #e5e5ea; border-radius:8px; padding:4px 12px; min-height:28px; color:#1d1d1f; }
            QLineEdit:hover { background:#e8e8ed; }
            QLineEdit:focus { border-color:#007aff; background:#fff; }
            QPushButton { background:#f2f2f7; border:1px solid #e5e5ea; border-radius:8px; padding:4px 16px; min-height:28px; color:#1d1d1f; }
            QPushButton:hover { background:#e8e8ed; }
            QPushButton:pressed { background:#dcdce3; }
            QPushButton:disabled { background:#f2f2f7; color:#aeaeb2; border-color:#e5e5ea; }
            QPushButton#primaryButton { background:#007aff; color:#fff; border:none; font-weight:600; border-radius:8px; }
            QPushButton#primaryButton:hover { background:#0056d6; }
            QPushButton#secondaryButton { background:#f2f2f7; border:1px solid #e5e5ea; color:#1d1d1f; border-radius:8px; }
            QPushButton#secondaryButton:hover { background:#e8e8ed; }
            QPushButton#smallButton { background:#f2f2f7; border:1px solid #e5e5ea; color:#1d1d1f; padding:2px 10px; min-height:24px; border-radius:6px; }
            QPushButton#smallButton:hover { background:#e8e8ed; }
            QPushButton#dangerButton { background:#fff; border:1px solid #ff3b30; color:#ff3b30; border-radius:8px; }
            QPushButton#dangerButton:hover { background:#ffeeed; }
            QProgressBar { background:#e5e5ea; border:none; border-radius:6px; text-align:center; color:#6e6e73; font-weight:600; height:20px; }
            QProgressBar::chunk { background:#007aff; border-radius:6px; }
            QStatusBar { background:#f5f5f7; border-top:1px solid #e5e5ea; color:#8e8e93; }
            QScrollBar:vertical { background:#f5f5f7; width:6px; border-radius:3px; }
            QScrollBar::handle:vertical { background:#c7c7cc; border-radius:3px; min-height:24px; }
            QScrollBar::handle:vertical:hover { background:#a1a1a6; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
            QScrollBar:horizontal { background:#f5f5f7; height:6px; border-radius:3px; }
            QScrollBar::handle:horizontal { background:#c7c7cc; border-radius:3px; min-width:24px; }
            QScrollBar::handle:horizontal:hover { background:#a1a1a6; }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
            QTabWidget::pane { border:1px solid #e5e5ea; border-radius:8px; background:#fff; }
            QTabBar::tab { background:#f2f2f7; border:1px solid #e5e5ea; padding:6px 18px; margin-right:2px; border-top-left-radius:8px; border-top-right-radius:8px; color:#6e6e73; }
            QTabBar::tab:selected { background:#fff; color:#007aff; font-weight:600; border-bottom:2px solid #007aff; }
            QTabBar::tab:hover { background:#e8e8ed; }
            QListWidget { background:#fff; border:1px solid #e5e5ea; border-radius:10px; padding:4px; }
            QListWidget::item:selected { background:#007aff; color:#fff; border-radius:5px; }
            QListWidget::item:hover:!selected { background:#e8f0fe; color:#007aff; }
        )");
    }

    // ========== 暗夜模式 ==========
    if (name == QStringLiteral("暗夜模式")) {
        return base() + QStringLiteral(R"(
            QMainWindow, QDialog { background-color: #1a1a2e; }
            QWidget { color: #eaeaea; }
            QGroupBox { background: #16213e; border:1px solid #0f3460; border-radius:10px; margin-top:14px; padding-top:12px; font-weight:bold; color:#eaeaea; }
            QGroupBox::title { left:12px; padding:0 6px; color:#4a90d9; }
            QTableView { background:#16213e; border:1px solid #0f3460; border-radius:8px; gridline-color:#0f3460; alternate-background-color:#1a2745; color:#eaeaea; }
            QTableView::item:selected { background:#0f3460; color:#e94560; }
            QHeaderView::section { background:#0f3460; color:#eaeaea; padding:8px 10px; border:none; border-bottom:2px solid #e94560; font-weight:bold; font-size:11px; }
            QComboBox { background:#16213e; border:1px solid #0f3460; border-radius:6px; padding:4px 10px; min-height:28px; color:#eaeaea; }
            QComboBox:hover { border-color:#e94560; }
            QComboBox::drop-down { border:none; width:20px; }
            QComboBox::down-arrow { border-left:4px solid transparent; border-right:4px solid transparent; border-top:5px solid #7f8c9b; margin-right:6px; }
            QComboBox QAbstractItemView { background:#16213e; border:1px solid #0f3460; border-radius:6px; color:#eaeaea; selection-background-color:#0f3460; selection-color:#e94560; }
            QLineEdit { background:#16213e; border:1px solid #0f3460; border-radius:6px; padding:4px 10px; min-height:28px; color:#eaeaea; }
            QLineEdit:hover { border-color:#e94560; }
            QLineEdit:focus { border-color:#e94560; }
            QPushButton { background:#16213e; border:1px solid #0f3460; border-radius:6px; padding:4px 14px; min-height:28px; color:#eaeaea; }
            QPushButton:hover { background:#1a3a5c; border-color:#e94560; color:#e94560; }
            QPushButton:pressed { background:#0f3460; }
            QPushButton:disabled { background:#1a1a2e; color:#666; border-color:#333; }
            QPushButton#primaryButton { background:#e94560; color:#fff; border:none; font-weight:bold; }
            QPushButton#primaryButton:hover { background:#ff6b81; }
            QPushButton#secondaryButton { background:#16213e; border:1px solid #0f3460; color:#eaeaea; }
            QPushButton#secondaryButton:hover { background:#1a3a5c; border-color:#e94560; color:#e94560; }
            QPushButton#smallButton { background:#16213e; border:1px solid #0f3460; color:#eaeaea; padding:2px 10px; min-height:24px; }
            QPushButton#smallButton:hover { background:#1a3a5c; border-color:#e94560; }
            QPushButton#dangerButton { background:#16213e; border:1px solid #e94560; color:#e94560; }
            QPushButton#dangerButton:hover { background:#2a1a1a; }
            QProgressBar { background:#0f3460; border:none; border-radius:6px; text-align:center; color:#eaeaea; font-weight:bold; height:20px; }
            QProgressBar::chunk { background:#e94560; border-radius:6px; }
            QStatusBar { background:#16213e; border-top:1px solid #0f3460; color:#7f8c9b; }
            QScrollBar:vertical { background:#1a1a2e; width:8px; border-radius:4px; }
            QScrollBar::handle:vertical { background:#0f3460; border-radius:4px; min-height:24px; }
            QScrollBar::handle:vertical:hover { background:#e94560; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
            QScrollBar:horizontal { background:#1a1a2e; height:8px; border-radius:4px; }
            QScrollBar::handle:horizontal { background:#0f3460; border-radius:4px; min-width:24px; }
            QScrollBar::handle:horizontal:hover { background:#e94560; }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
            QTabWidget::pane { border:1px solid #0f3460; border-radius:6px; background:#16213e; }
            QTabBar::tab { background:#1a1a2e; border:1px solid #0f3460; padding:6px 18px; margin-right:2px; border-top-left-radius:6px; border-top-right-radius:6px; color:#888; }
            QTabBar::tab:selected { background:#16213e; color:#e94560; font-weight:bold; border-bottom:2px solid #e94560; }
            QTabBar::tab:hover { background:#0f3460; color:#eaeaea; }
            QListWidget { background:#16213e; border:1px solid #0f3460; border-radius:8px; padding:4px; color:#eaeaea; }
            QListWidget::item:selected { background:#e94560; color:#fff; font-weight:bold; }
            QListWidget::item:hover:!selected { background:#1a3a5c; color:#e94560; }
            QSpinBox, QDoubleSpinBox { background:#16213e; border:1px solid #0f3460; color:#eaeaea; padding:3px 24px 3px 8px; min-height:26px; border-radius:4px; }
            QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-position: top right; subcontrol-origin: margin; background:#0f3460; }
            QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-position: bottom right; subcontrol-origin: margin; background:#0f3460; }
        )");
    }

    // ========== 渐变紫 ==========
    if (name == QStringLiteral("渐变紫")) {
        return base() + QStringLiteral(R"(
            QMainWindow, QDialog { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #f8f6fc,stop:1 #ede4f6); }
            QWidget { color: #2d1b4e; }
            QGroupBox { background: rgba(255,255,255,0.7); border:1px solid #d8c8f0; border-radius:12px; margin-top:14px; padding-top:14px; font-weight:bold; color:#4a1d8f; }
            QGroupBox::title { left:12px; padding:0 6px; color:#6b21a8; }
            QTableView { background: rgba(255,255,255,0.8); border:1px solid #d8c8f0; border-radius:10px; gridline-color:#e8dcf5; alternate-background-color:rgba(248,244,252,0.6); }
            QTableView::item:selected { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #e9d5ff,stop:1 #d8b4fe); color:#4a1d8f; }
            QHeaderView::section { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #7c3aed,stop:1 #6d28d9); color:#fff; padding:8px 10px; border:none; border-bottom:2px solid #a78bfa; font-weight:bold; font-size:11px; }
            QComboBox { background: rgba(255,255,255,0.8); border:1px solid #d8c8f0; border-radius:8px; padding:4px 10px; min-height:28px; color:#2d1b4e; }
            QComboBox:hover { border-color:#a78bfa; }
            QComboBox:focus { border-color:#7c3aed; }
            QComboBox::drop-down { border:none; width:20px; }
            QComboBox::down-arrow { border-left:4px solid transparent; border-right:4px solid transparent; border-top:5px solid #7c3aed; margin-right:6px; }
            QComboBox QAbstractItemView { background:#fff; border:1px solid #d8c8f0; border-radius:8px; selection-background-color:#e9d5ff; selection-color:#4a1d8f; }
            QLineEdit { background: rgba(255,255,255,0.8); border:1px solid #d8c8f0; border-radius:8px; padding:4px 10px; min-height:28px; color:#2d1b4e; }
            QLineEdit:hover { border-color:#a78bfa; }
            QLineEdit:focus { border-color:#7c3aed; }
            QPushButton { background: rgba(255,255,255,0.7); border:1px solid #d8c8f0; border-radius:8px; padding:4px 14px; min-height:28px; color:#4a1d8f; }
            QPushButton:hover { background:#ede4f6; border-color:#a78bfa; color:#6b21a8; }
            QPushButton:pressed { background:#d8c8f0; }
            QPushButton:disabled { background:#f2f2f5; color:#b0b0c0; border-color:#ddd; }
            QPushButton#primaryButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #7c3aed,stop:1 #6d28d9); color:#fff; border:none; font-weight:bold; border-radius:8px; }
            QPushButton#primaryButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #8b5cf6,stop:1 #7c3aed); }
            QPushButton#secondaryButton { background: rgba(255,255,255,0.7); border:1px solid #d8c8f0; color:#4a1d8f; border-radius:8px; }
            QPushButton#secondaryButton:hover { background:#ede4f6; border-color:#a78bfa; }
            QPushButton#smallButton { background: rgba(255,255,255,0.7); border:1px solid #d8c8f0; color:#4a1d8f; padding:2px 10px; min-height:24px; }
            QPushButton#smallButton:hover { background:#ede4f6; border-color:#a78bfa; }
            QPushButton#dangerButton { background:#fff; border:1px solid #ef4444; color:#dc2626; border-radius:8px; }
            QPushButton#dangerButton:hover { background:#fef2f2; }
            QProgressBar { background:#e8dcf5; border:none; border-radius:6px; text-align:center; color:#4a1d8f; font-weight:bold; height:20px; }
            QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #7c3aed,stop:1 #a78bfa); border-radius:6px; }
            QStatusBar { background: rgba(255,255,255,0.6); border-top:1px solid #d8c8f0; color:#6b21a8; }
            QScrollBar:vertical { background:rgba(255,255,255,0.4); width:8px; border-radius:4px; }
            QScrollBar::handle:vertical { background:#c4b5e0; border-radius:4px; min-height:24px; }
            QScrollBar::handle:vertical:hover { background:#7c3aed; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
            QScrollBar:horizontal { background:rgba(255,255,255,0.4); height:8px; border-radius:4px; }
            QScrollBar::handle:horizontal { background:#c4b5e0; border-radius:4px; min-width:24px; }
            QScrollBar::handle:horizontal:hover { background:#7c3aed; }
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width:0; }
            QTabWidget::pane { border:1px solid #d8c8f0; border-radius:8px; background:rgba(255,255,255,0.7); }
            QTabBar::tab { background:rgba(255,255,255,0.4); border:1px solid #d8c8f0; padding:6px 18px; margin-right:2px; border-top-left-radius:8px; border-top-right-radius:8px; color:#6b21a8; }
            QTabBar::tab:selected { background:rgba(255,255,255,0.8); color:#7c3aed; font-weight:bold; border-bottom:2px solid #7c3aed; }
            QTabBar::tab:hover { background:#ede4f6; }
            QListWidget { background:rgba(255,255,255,0.7); border:1px solid #d8c8f0; border-radius:10px; padding:4px; }
            QListWidget::item:selected { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #7c3aed,stop:1 #6d28d9); color:#fff; font-weight:bold; border-radius:5px; }
            QListWidget::item:hover:!selected { background:#ede4f6; color:#6b21a8; }
        )");
    }

    return base(); // fallback
}
