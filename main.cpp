#include "mainwindow.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
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
