#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QApplication>
#include "wifimanager.h"
#include "config_dialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowConfig();
    void onStartAutoReconnect();
    void onStopAutoReconnect();
    void onQuit();
    void onConnectionStatusChanged(bool connected);
    void onLoginResult(bool success, const QString &message);
    void showUserFriendlyError(const QString &title, const QString &message);
    void setupAutoStart();

private:
    void createTrayIcon();
    void createConfigDialog();
    void checkFirstRun();
    void startBackgroundMode();

    Ui::MainWindow *ui;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_showConfigAction;
    QAction *m_startAction;
    QAction *m_stopAction;
    QAction *m_quitAction;
    
    ConfigDialog *m_configDialog;
    WiFiManager *m_wifiManager;
    
    bool m_isFirstRun;
    bool m_backgroundMode;
    bool m_isConfigDialogOpen;  // 配置对话框是否打开
};

#endif // MAINWINDOW_H
