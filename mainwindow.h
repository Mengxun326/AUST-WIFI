#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QSettings>
#include "wifimanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    void loadConfig();
    void saveConfig();

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    QVBoxLayout *m_mainLayout;
    QLabel *m_userLabel;
    QLineEdit *m_userEdit;
    QLabel *m_passwordLabel;
    QLineEdit *m_passwordEdit;
    QLabel *m_serverLabel;
    QComboBox *m_serverCombo;
    QCheckBox *m_autoStartCheck;
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
    
    QSettings *m_settings;
};

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
};

#endif // MAINWINDOW_H
