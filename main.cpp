#include "mainwindow.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QSettings>
#include <QStyleHints>
#include <QPalette>
#include <QStyleFactory>

static QPalette createDarkPalette()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor(45, 45, 45));
    p.setColor(QPalette::WindowText,      QColor(208, 208, 208));
    p.setColor(QPalette::Base,            QColor(30, 30, 30));
    p.setColor(QPalette::AlternateBase,   QColor(45, 45, 45));
    p.setColor(QPalette::ToolTipBase,     QColor(45, 45, 45));
    p.setColor(QPalette::ToolTipText,     QColor(208, 208, 208));
    p.setColor(QPalette::Text,            QColor(208, 208, 208));
    p.setColor(QPalette::Button,          QColor(45, 45, 45));
    p.setColor(QPalette::ButtonText,      QColor(208, 208, 208));
    p.setColor(QPalette::BrightText,      QColor(255, 100, 100));
    p.setColor(QPalette::Link,            QColor(42, 130, 218));
    p.setColor(QPalette::Highlight,       QColor(42, 130, 218));
    p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(128, 128, 128));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(128, 128, 128));
    return p;
}

static QPalette createLightPalette()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor(243, 243, 243));
    p.setColor(QPalette::WindowText,      QColor(32, 32, 32));
    p.setColor(QPalette::Base,            QColor(255, 255, 255));
    p.setColor(QPalette::AlternateBase,   QColor(243, 243, 243));
    p.setColor(QPalette::ToolTipBase,     QColor(255, 255, 255));
    p.setColor(QPalette::ToolTipText,     QColor(32, 32, 32));
    p.setColor(QPalette::Text,            QColor(32, 32, 32));
    p.setColor(QPalette::Button,          QColor(243, 243, 243));
    p.setColor(QPalette::ButtonText,      QColor(32, 32, 32));
    p.setColor(QPalette::BrightText,      QColor(255, 0, 0));
    p.setColor(QPalette::Link,            QColor(0, 120, 212));
    p.setColor(QPalette::Highlight,       QColor(0, 120, 212));
    p.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(160, 160, 160));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(160, 160, 160));
    return p;
}

static void applyTheme(QApplication &app)
{
    bool isDark = (app.styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    app.setPalette(isDark ? createDarkPalette() : createLightPalette());
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 使用 Fusion 样式以支持深色模式 QPalette
    a.setStyle(QStyleFactory::create("Fusion"));
    applyTheme(a);
    QObject::connect(a.styleHints(), &QStyleHints::colorSchemeChanged, [&a]() {
        applyTheme(a);
    });

    // 检查系统托盘是否可用
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "错误", "系统托盘不可用，程序无法正常运行！");
        return -1;
    }

    // 设置应用程序信息
    QApplication::setApplicationName("AUST WiFi 自动重连工具");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("AUST");
    QApplication::setOrganizationDomain("aust.edu.cn");

    MainWindow w;

    // 如果是首次运行，显示窗口；否则直接隐藏到系统托盘
    QSettings settings("AUST_WIFI", "Config");
    bool hasStudentConfig = settings.contains("student/user");
    bool hasTeacherConfig = settings.contains("teacher/user");
    bool hasOldConfig = settings.contains("user");

    if (!hasStudentConfig && !hasTeacherConfig && !hasOldConfig) {
        w.show();
    }

    return a.exec();
}
