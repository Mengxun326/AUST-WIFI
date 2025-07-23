#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QMessageBox>
#include <QStyle>

// MainWindow 实现
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_configDialog(nullptr)
    , m_wifiManager(nullptr)
    , m_isFirstRun(false)
    , m_backgroundMode(false)
    , m_isConfigDialogOpen(false)
{
    ui->setupUi(this);
    setWindowTitle("AUST WiFi 自动重连工具");
    
    // 设置状态栏显示个人信息
    statusBar()->showMessage("信息安全23-1 王智杰");
    
    // 检查是否首次运行
    checkFirstRun();
    
    // 创建WiFi管理器（在主线程中）
    m_wifiManager = new WiFiManager(this);
    
    connect(m_wifiManager, &WiFiManager::connectionStatusChanged, 
            this, &MainWindow::onConnectionStatusChanged);
    connect(m_wifiManager, &WiFiManager::loginResult, 
            this, &MainWindow::onLoginResult);
    
    // 创建系统托盘
    createTrayIcon();
    
    // 创建配置对话框
    createConfigDialog();
    
    // 连接UI按钮信号
    connect(ui->configButton, &QPushButton::clicked, this, &MainWindow::onShowConfig);
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartAutoReconnect);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopAutoReconnect);
    
    // 如果是首次运行，显示配置对话框
    if (m_isFirstRun) {
        if (m_configDialog->exec() == QDialog::Accepted) {
            setupAutoStart();
            startBackgroundMode();
        } else {
            QApplication::quit();
        }
    } else {
        // 直接启动后台模式
        m_backgroundMode = true;
        hide();
        // 延迟启动自动重连，避免启动时崩溃
        QTimer::singleShot(2000, [this]() {
            m_wifiManager->startAutoReconnect();
        });
    }
    
    // 确保网络检测功能可用，以便配置界面可以暂停操作
    // 如果还没有启动自动重连，则启动一个基础的网络检测
    QTimer::singleShot(3000, [this]() {
        if (!m_wifiManager || m_wifiManager->isConfiguring()) {
            return;  // 如果正在配置中，跳过
        }
        // 确保网络检测已启动，这样打开配置界面时就有操作可以暂停
        m_wifiManager->startAutoReconnect();
        qDebug() << "确保网络检测功能已启动，以支持配置界面的暂停功能";
    });
}

MainWindow::~MainWindow()
{
    qDebug() << "开始析构MainWindow...";
    
    // 断开所有信号连接，防止析构时的信号问题
    if (m_wifiManager) {
        qDebug() << "断开WiFiManager信号连接...";
        disconnect(m_wifiManager, nullptr, this, nullptr);
        m_wifiManager->stopAutoReconnect();
        m_wifiManager->deleteLater();
        m_wifiManager = nullptr;
    }
    
    // 清理托盘图标及其相关对象
    if (m_trayIcon) {
        qDebug() << "清理系统托盘图标...";
        m_trayIcon->hide();
        if (m_trayMenu) {
            m_trayMenu->deleteLater();
            m_trayMenu = nullptr;
        }
        m_trayIcon->deleteLater();
        m_trayIcon = nullptr;
    }
    
    // 清理配置对话框
    if (m_configDialog) {
        qDebug() << "清理配置对话框...";
        m_configDialog->deleteLater();
        m_configDialog = nullptr;
    }
    
    qDebug() << "清理UI...";
    delete ui;
    ui = nullptr;
    
    qDebug() << "MainWindow析构完成";
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_backgroundMode) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::checkFirstRun()
{
    QSettings settings("AUST_WIFI", "Config");
    bool hasStudentConfig = settings.contains("student/user");
    bool hasTeacherConfig = settings.contains("teacher/user");
    bool hasOldConfig = settings.contains("user");  // 兼容旧版本
    
    m_isFirstRun = !hasStudentConfig && !hasTeacherConfig && !hasOldConfig;
    
    if (hasOldConfig && !hasStudentConfig && !hasTeacherConfig) {
        qDebug() << "检测到旧版本配置，需要升级到双配置系统";
    }
}

void MainWindow::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    
    // 设置初始图标 - 优先使用资源文件中的图标，然后检查本地文件，最后回退到系统图标
    QIcon appIcon;
    if (QFile::exists(":/icons/app.ico")) {
        appIcon = QIcon(":/icons/app.ico");
        qDebug() << "使用资源文件中的图标: :/icons/app.ico";
    } else if (QFile::exists("icons/app.ico")) {
        appIcon = QIcon("icons/app.ico");
        qDebug() << "使用本地文件图标: icons/app.ico";
    } else {
        appIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
        qDebug() << "使用系统默认图标";
    }
    
    m_trayIcon->setIcon(appIcon);
    m_trayIcon->setToolTip("AUST WiFi 自动重连工具");
    
    // 设置窗口图标
    setWindowIcon(appIcon);
    
    m_trayMenu = new QMenu();
    
    m_showConfigAction = new QAction("配置", this);
    m_startAction = new QAction("开始自动重连", this);
    m_stopAction = new QAction("停止自动重连", this);
    m_quitAction = new QAction("退出", this);
    
    m_trayMenu->addAction(m_showConfigAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_startAction);
    m_trayMenu->addAction(m_stopAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, 
            this, &MainWindow::onTrayIconActivated);
    connect(m_showConfigAction, &QAction::triggered, 
            this, &MainWindow::onShowConfig);
    connect(m_startAction, &QAction::triggered, 
            this, &MainWindow::onStartAutoReconnect);
    connect(m_stopAction, &QAction::triggered, 
            this, &MainWindow::onStopAutoReconnect);
    connect(m_quitAction, &QAction::triggered, 
            this, &MainWindow::onQuit);
    
    m_trayIcon->show();
}

void MainWindow::createConfigDialog()
{
    m_configDialog = new ConfigDialog(this);
}

void MainWindow::startBackgroundMode()
{
    m_backgroundMode = true;
    hide();
    m_wifiManager->startAutoReconnect();
}

void MainWindow::setupAutoStart()
{
    QSettings settings("AUST_WIFI", "Config");
    if (settings.value("autoStart", false).toBool()) {
        QSettings autoStartSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                                  QSettings::NativeFormat);
        QString appPath = QApplication::applicationFilePath();
        appPath = QDir::toNativeSeparators(appPath);
        autoStartSettings.setValue("AUST_WIFI", appPath);
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
        }
    }
}

void MainWindow::onShowConfig()
{
    qDebug() << "=== 打开配置界面 ===";
    
    if (m_wifiManager) {
        qDebug() << "暂停网络操作前的状态：";
        m_wifiManager->printCurrentStatus();
    }
    
    // 暂停网络操作
    m_isConfigDialogOpen = true;
    
    if (m_wifiManager) {
        m_wifiManager->pauseNetworkOperations();
        qDebug() << "暂停网络操作后的状态：";
        m_wifiManager->printCurrentStatus();
    } else {
        qDebug() << "错误：WiFi管理器为空，无法暂停网络操作";
    }
    
    qDebug() << "显示配置对话框...";
    m_configDialog->loadConfig();
    int result = m_configDialog->exec();
    qDebug() << "配置对话框已关闭，结果:" << (result == QDialog::Accepted ? "已保存" : "未保存");
    
    // 恢复网络操作
    m_isConfigDialogOpen = false;
    qDebug() << "恢复网络操作...";
    
    if (m_wifiManager) {
        m_wifiManager->resumeNetworkOperations();
        qDebug() << "恢复网络操作后的状态：";
        m_wifiManager->printCurrentStatus();
    } else {
        qDebug() << "错误：WiFi管理器为空，无法恢复网络操作";
    }
    
    if (result == QDialog::Accepted) {
        qDebug() << "配置已保存，设置自动启动并延迟检测网络";
        setupAutoStart();
        // 配置更新后立即进行一次连接检测
        QTimer::singleShot(2000, m_wifiManager, &WiFiManager::checkInternetConnection);
    } else {
        qDebug() << "配置未保存";
    }
    
    qDebug() << "=== 配置界面操作完成 ===";
}

void MainWindow::onStartAutoReconnect()
{
    m_wifiManager->startAutoReconnect();
    m_trayIcon->showMessage("AUST WiFi", "自动重连已启动", 
                           QSystemTrayIcon::Information, 2000);
}

void MainWindow::onStopAutoReconnect()
{
    m_wifiManager->stopAutoReconnect();
    m_trayIcon->showMessage("AUST WiFi", "自动重连已停止", 
                           QSystemTrayIcon::Information, 2000);
}

void MainWindow::onQuit()
{
    m_wifiManager->stopAutoReconnect();
    QApplication::quit();
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    QIcon statusIcon;
    
    if (connected) {
        m_trayIcon->setToolTip("AUST WiFi - 已连接");
        
        // 优先使用资源文件图标，然后本地文件，最后回退到系统图标
        if (QFile::exists(":/icons/connected.ico")) {
            statusIcon = QIcon(":/icons/connected.ico");
        } else if (QFile::exists("icons/connected.ico")) {
            statusIcon = QIcon("icons/connected.ico");
        } else {
            statusIcon = QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton);
        }
        
        ui->statusLabel->setText("状态: 网络连接正常");
        ui->statusLabel->setStyleSheet("color: #107c10; font-weight: bold;");
    } else {
        m_trayIcon->setToolTip("AUST WiFi - 未连接");
        
        // 优先使用资源文件图标，然后本地文件，最后回退到系统图标
        if (QFile::exists(":/icons/disconnected.ico")) {
            statusIcon = QIcon(":/icons/disconnected.ico");
        } else if (QFile::exists(":/icons/warning.ico")) {
            statusIcon = QIcon(":/icons/warning.ico");
        } else if (QFile::exists("icons/disconnected.ico")) {
            statusIcon = QIcon("icons/disconnected.ico");
        } else if (QFile::exists("icons/warning.ico")) {
            statusIcon = QIcon("icons/warning.ico");
        } else {
            statusIcon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
        }
        
        ui->statusLabel->setText("状态: 网络异常，正在重连...");
        ui->statusLabel->setStyleSheet("color: #d13438; font-weight: bold;");
    }
    
    m_trayIcon->setIcon(statusIcon);
}

void MainWindow::onLoginResult(bool success, const QString &message)
{
    if (success) {
        m_trayIcon->showMessage("AUST WiFi", "登录成功", 
                               QSystemTrayIcon::Information, 2000);
        qDebug() << "登录成功:" << message;
    } else {
        m_trayIcon->showMessage("AUST WiFi", "登录失败: " + message, 
                               QSystemTrayIcon::Warning, 3000);
        qDebug() << "登录失败:" << message;
        
        // 如果包含配置相关的错误，显示用户友好提示
        if (message.contains("未配置") || message.contains("配置") || message.contains("账号信息")) {
            showUserFriendlyError("配置错误", message + "\n\n是否现在打开配置界面？");
        } else if (message.contains("账号类型不匹配") || message.contains("格式不正确")) {
            showUserFriendlyError("账号错误", message + "\n\n请检查您的账号配置是否正确。");
        } else if (message.contains("网络") || message.contains("连接")) {
            showUserFriendlyError("网络错误", message + "\n\n请检查网络连接或稍后重试。");
        } else if (message.contains("Operation canceled") || message.contains("取消")) {
            // 操作取消不需要显示错误提示
            qDebug() << "操作被取消，跳过错误提示";
        } else {
            // 其他类型的错误也显示给用户
            showUserFriendlyError("登录失败", message);
        }
    }
}

void MainWindow::showUserFriendlyError(const QString &title, const QString &message)
{
    // 只有在非配置状态下才显示错误提示，避免在配置过程中干扰用户
    if (m_isConfigDialogOpen) {
        qDebug() << "跳过错误提示 - 用户正在配置中";
        return;
    }
    
    qDebug() << "显示用户友好错误提示:" << title << "-" << message;
    
    // 如果包含配置相关的错误，询问是否打开配置
    if (message.contains("是否现在打开配置界面")) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            title, 
            message,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        
        if (reply == QMessageBox::Yes) {
            // 延迟打开配置界面，避免在错误处理过程中立即打开
            QTimer::singleShot(500, this, &MainWindow::onShowConfig);
        }
    } else {
        // 普通错误提示
        QMessageBox::warning(this, title, message);
    }
}
