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
#include <QTimer>
#include <QDateTime>

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
    , m_isAutoReconnectActive(false)
    , m_wifiInfoTimer(nullptr)
    , m_moveEndTimer(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("AUST WiFi 自动重连工具");
    
    // 设置状态栏显示个人信息（Windows 11风格）
    statusBar()->showMessage("信息安全23-1 王智杰 | AUST WiFi 自动重连工具 v3.0");
    
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
    connect(ui->toggleButton, &QPushButton::clicked, this, &MainWindow::onToggleAutoReconnect);
    
    // 初始化切换按钮状态
    updateToggleButton();
    
    // 初始化状态显示
    ui->statusIndicator->setStyleSheet("background-color: #FFB900;");
    ui->statusIndicator->setProperty("class", "connecting");
    ui->statusValueLabel->setText("正在检测...");
    ui->statusDescriptionLabel->setText("系统正在检测您的网络连接状态");
    
    // 初始化WiFi信息显示
    updateWifiInfo();
    
    // 创建定时器定期更新WiFi信息（每5秒更新一次）
    m_wifiInfoTimer = new QTimer(this);
    connect(m_wifiInfoTimer, &QTimer::timeout, this, &MainWindow::updateWifiInfo);
    m_wifiInfoTimer->start(5000);
    
    // 连接WiFi SSID更新信号，当SSID更新时自动刷新UI
    connect(m_wifiManager, &WiFiManager::wifiSSIDUpdated, this, &MainWindow::updateWifiInfo);
    
    // 创建窗口移动结束检测定时器（用于在移动结束后恢复更新）
    m_moveEndTimer = new QTimer(this);
    m_moveEndTimer->setSingleShot(true);
    m_moveEndTimer->setInterval(300);  // 移动结束后300ms再恢复更新
    connect(m_moveEndTimer, &QTimer::timeout, [this]() {
        if (m_wifiInfoTimer && !m_wifiInfoTimer->isActive()) {
            m_wifiInfoTimer->start(5000);
            updateWifiInfo();  // 立即更新一次
        }
    });
    
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
            m_isAutoReconnectActive = true;
            updateToggleButton();
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

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    
    // 窗口移动时暂停WiFi信息更新，避免卡顿
    if (m_wifiInfoTimer && m_wifiInfoTimer->isActive()) {
        m_wifiInfoTimer->stop();
    }
    
    // 重置移动结束检测定时器
    if (m_moveEndTimer) {
        m_moveEndTimer->stop();
        m_moveEndTimer->start();
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
            this, &MainWindow::onToggleAutoReconnect);
    connect(m_stopAction, &QAction::triggered, 
            this, &MainWindow::onToggleAutoReconnect);
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
    m_isAutoReconnectActive = true;
    updateToggleButton();
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

void MainWindow::onToggleAutoReconnect()
{
    if (m_isAutoReconnectActive) {
        // 当前是激活状态，停止自动重连
        m_wifiManager->stopAutoReconnect();
        m_isAutoReconnectActive = false;
        m_trayIcon->showMessage("AUST WiFi", "自动重连已停止", 
                               QSystemTrayIcon::Information, 2000);
    } else {
        // 当前是停止状态，启动自动重连
        m_wifiManager->startAutoReconnect();
        m_isAutoReconnectActive = true;
        m_trayIcon->showMessage("AUST WiFi", "自动重连已启动", 
                               QSystemTrayIcon::Information, 2000);
    }
    updateToggleButton();
}

void MainWindow::updateToggleButton()
{
    if (m_isAutoReconnectActive) {
        // 激活状态：显示"停止自动重连"，使用停止样式
        ui->toggleButton->setText("停止自动重连");
        ui->toggleButton->setProperty("class", "stopped");
        ui->toggleButton->style()->unpolish(ui->toggleButton);
        ui->toggleButton->style()->polish(ui->toggleButton);
    } else {
        // 停止状态：显示"开始自动重连"，使用开始样式
        ui->toggleButton->setText("开始自动重连");
        ui->toggleButton->setProperty("class", "");
        ui->toggleButton->style()->unpolish(ui->toggleButton);
        ui->toggleButton->style()->polish(ui->toggleButton);
    }
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
        
        // 更新状态指示器
        ui->statusIndicator->setStyleSheet("background-color: #107C10;");
        ui->statusIndicator->setProperty("class", "connected");
        ui->statusIndicator->style()->unpolish(ui->statusIndicator);
        ui->statusIndicator->style()->polish(ui->statusIndicator);
        
        // 更新状态文本
        ui->statusValueLabel->setText("已连接");
        ui->statusDescriptionLabel->setText("网络连接正常，可以正常使用");
        
        // 优先使用资源文件图标，然后本地文件，最后回退到系统图标
        if (QFile::exists(":/icons/connected.ico")) {
            statusIcon = QIcon(":/icons/connected.ico");
        } else if (QFile::exists("icons/connected.ico")) {
            statusIcon = QIcon("icons/connected.ico");
        } else {
            statusIcon = QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton);
        }
    } else {
        m_trayIcon->setToolTip("AUST WiFi - 未连接");
        
        // 更新状态指示器
        ui->statusIndicator->setStyleSheet("background-color: #D13438;");
        ui->statusIndicator->setProperty("class", "disconnected");
        ui->statusIndicator->style()->unpolish(ui->statusIndicator);
        ui->statusIndicator->style()->polish(ui->statusIndicator);
        
        // 更新状态文本
        ui->statusValueLabel->setText("未连接");
        ui->statusDescriptionLabel->setText("网络连接异常，正在尝试重连...");
        
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
    }
    
    m_trayIcon->setIcon(statusIcon);
    
    // 更新WiFi信息
    updateWifiInfo();
}

void MainWindow::onLoginResult(bool success, const QString &message)
{
    if (success) {
        // 记录登录成功时间（使用ISO格式确保兼容性）
        QSettings settings("AUST_WIFI", "Config");
        QDateTime currentTime = QDateTime::currentDateTime();
        settings.setValue("lastLoginTime", currentTime);
        settings.sync();
        qDebug() << "保存最后登录时间:" << currentTime.toString(Qt::ISODate);
        
        // 更新WiFi信息显示
        updateWifiInfo();
        
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

void MainWindow::updateWifiInfo()
{
    if (!m_wifiManager) {
        return;
    }
    
    // 获取当前WiFi SSID（从缓存，非阻塞）
    QString ssid = m_wifiManager->getCurrentWifiSSID();
    // 异步更新SSID（不阻塞UI）
    m_wifiManager->updateWifiSSIDAsync();
    if (ssid.isEmpty()) {
        ui->wifiNameValue->setText("未检测到");
    } else {
        ui->wifiNameValue->setText(ssid);
    }
    
    // 获取用户类型
    QString userType = m_wifiManager->determineUserTypeBySSID(ssid);
    if (userType == "student") {
        ui->userTypeValue->setText("学生");
    } else if (userType == "teacher") {
        ui->userTypeValue->setText("教师");
    } else {
        ui->userTypeValue->setText("未检测到");
    }
    
    // 更新最后登录时间（从设置中读取）
    QSettings settings("AUST_WIFI", "Config");
    QVariant lastLoginVariant = settings.value("lastLoginTime");
    QDateTime lastLogin;
    
    // 尝试多种方式读取时间
    if (lastLoginVariant.isValid()) {
        if (lastLoginVariant.type() == QVariant::DateTime) {
            lastLogin = lastLoginVariant.toDateTime();
        } else if (lastLoginVariant.type() == QVariant::String) {
            // 如果是字符串格式，尝试解析
            QString timeStr = lastLoginVariant.toString();
            lastLogin = QDateTime::fromString(timeStr, Qt::ISODate);
            if (!lastLogin.isValid()) {
                lastLogin = QDateTime::fromString(timeStr, "yyyy-MM-dd hh:mm:ss");
            }
        }
    }
    
    if (lastLogin.isValid() && lastLogin <= QDateTime::currentDateTime()) {
        QDateTime now = QDateTime::currentDateTime();
        qint64 seconds = lastLogin.secsTo(now);
        
        if (seconds < 0) {
            // 时间在未来，可能是时区问题，使用当前时间
            ui->lastLoginValue->setText("刚刚");
        } else if (seconds < 60) {
            ui->lastLoginValue->setText(QString("刚刚（%1秒前）").arg(seconds));
        } else if (seconds < 3600) {
            ui->lastLoginValue->setText(QString("%1分钟前").arg(seconds / 60));
        } else if (seconds < 86400) {
            ui->lastLoginValue->setText(QString("%1小时前").arg(seconds / 3600));
        } else {
            ui->lastLoginValue->setText(lastLogin.toString("yyyy-MM-dd hh:mm"));
        }
    } else {
        ui->lastLoginValue->setText("从未登录");
    }
}
