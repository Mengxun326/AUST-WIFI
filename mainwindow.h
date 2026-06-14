#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QMoveEvent>
#include <QApplication>
#include <QTimer>
#include "wifimanager.h"
#include "config_dialog.h"
#include "updatemanager.h"

class QProgressDialog;

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
    void moveEvent(QMoveEvent *event) override;

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowConfig();
    void onToggleAutoReconnect();
    void onCopyDiagnostics();
    void onQuit();
    void onConnectionStatusChanged(bool connected);
    void onLoginResult(bool success, const QString &message);
    void showUserFriendlyError(const QString &title, const QString &message);
    void setupAutoStart();
    void onCheckUpdates();
    void onUpdateAvailable(const UpdateInfo &info, bool manual);
    void onNoUpdateAvailable(const QString &currentVersion, const QString &latestVersion, bool manual);
    void onUpdateCheckFailed(const QString &message, bool manual);
    void onUpdateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onUpdateDownloadFinished(const QString &installerPath);
    void onUpdateFailed(const QString &message);

private:
    void createTrayIcon();
    void createConfigDialog();
    void createUpdateManager();
    void checkFirstRun();
    void startBackgroundMode();
    void updateWifiInfo();
    void closeUpdateProgressDialog();

    Ui::MainWindow *ui;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_showConfigAction;
    QAction *m_checkUpdateAction;
    QAction *m_diagnosticsAction;
    QAction *m_startAction;
    QAction *m_stopAction;
    QAction *m_quitAction;
    
    ConfigDialog *m_configDialog;
    WiFiManager *m_wifiManager;
    UpdateManager *m_updateManager;
    QProgressDialog *m_updateProgressDialog;
    
    bool m_isFirstRun;
    bool m_backgroundMode;
    bool m_isConfigDialogOpen;  // 配置对话框是否打开
    bool m_isAutoReconnectActive;  // 自动重连是否激活
    QTimer *m_wifiInfoTimer;  // WiFi信息更新定时器
    QTimer *m_moveEndTimer;  // 窗口移动结束检测定时器
    void updateToggleButton();  // 更新切换按钮的状态和文本
};

#endif // MAINWINDOW_H
