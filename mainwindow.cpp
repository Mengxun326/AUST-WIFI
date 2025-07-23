#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QDebug>
#include <QFile> // Added for file existence check

// ConfigDialog 实现
ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , m_settings(new QSettings("AUST_WIFI", "Config", this))
{
    setWindowTitle("AUST WiFi 配置");
    setFixedSize(350, 380);
    setModal(true);
    
    // 设置对话框样式
    setStyleSheet(R"(
        QDialog {
            background-color: #f5f5f5;
        }
        
        QLabel {
            color: #333;
            font-size: 12px;
            font-weight: bold;
        }
        
        QLineEdit {
            border: 1px solid #ccc;
            border-radius: 4px;
            padding: 9px;
            font-size: 12px;
            background-color: white;
            min-height: 16px;
        }
        
        QLineEdit:focus {
            border-color: #0078d4;
        }
        
        QComboBox {
            border: 1px solid #ccc;
            border-radius: 4px;
            padding: 9px;
            font-size: 12px;
            background-color: white;
            min-height: 16px;
        }
        
        QComboBox:focus {
            border-color: #0078d4;
        }
        
        QCheckBox {
            font-size: 12px;
            color: #333;
        }
    )");
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(15);
    m_mainLayout->setContentsMargins(25, 25, 25, 25);
    
    // 学号输入
    m_userLabel = new QLabel("学号:", this);
    m_userEdit = new QLineEdit(this);
    m_userEdit->setPlaceholderText("请输入学号");
    m_mainLayout->addWidget(m_userLabel);
    m_mainLayout->addWidget(m_userEdit);
    
    // 密码输入
    m_passwordLabel = new QLabel("密码:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_mainLayout->addWidget(m_passwordLabel);
    m_mainLayout->addWidget(m_passwordEdit);
    
    // 运营商选择
    m_serverLabel = new QLabel("运营商:", this);
    m_serverCombo = new QComboBox(this);
    m_serverCombo->addItem("电信", "aust");
    m_serverCombo->addItem("联通", "unicom");
    m_serverCombo->addItem("移动", "cmcc");
    m_mainLayout->addWidget(m_serverLabel);
    m_mainLayout->addWidget(m_serverCombo);
    
    // 开机自启动选项
    m_autoStartCheck = new QCheckBox("开机自动启动", this);
    m_mainLayout->addWidget(m_autoStartCheck);
    
    // 个人信息标签
    QLabel *authorLabel = new QLabel("信息安全23-1 王智杰", this);
    authorLabel->setAlignment(Qt::AlignCenter);
    authorLabel->setMinimumHeight(25);  // 确保标签有足够的高度
    authorLabel->setStyleSheet(R"(
        QLabel {
            color: #666;
            font-size: 10px;
            font-weight: normal;
            margin-top: 15px;
            margin-bottom: 10px;
            padding: 5px;
            min-height: 20px;
            line-height: 20px;
        }
    )");
    m_mainLayout->addWidget(authorLabel);
    
    // 按钮区域
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(10);
    
    m_saveButton = new QPushButton("保存", this);
    m_saveButton->setStyleSheet(R"(
        QPushButton {
            background-color: #0078d4;
            border: none;
            border-radius: 4px;
            color: white;
            font-size: 12px;
            padding: 8px 16px;
            min-height: 24px;
        }
        QPushButton:hover {
            background-color: #106ebe;
        }
        QPushButton:pressed {
            background-color: #005a9e;
        }
    )");
    
    m_cancelButton = new QPushButton("取消", this);
    m_cancelButton->setStyleSheet(R"(
        QPushButton {
            background-color: #d13438;
            border: none;
            border-radius: 4px;
            color: white;
            font-size: 12px;
            padding: 8px 16px;
            min-height: 24px;
        }
        QPushButton:hover {
            background-color: #b52d31;
        }
        QPushButton:pressed {
            background-color: #9a272a;
        }
    )");
    
    m_buttonLayout->addWidget(m_saveButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigDialog::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ConfigDialog::onCancelClicked);
    
    loadConfig();
}

void ConfigDialog::loadConfig()
{
    m_userEdit->setText(m_settings->value("user").toString());
    m_passwordEdit->setText(m_settings->value("password").toString());
    
    QString server = m_settings->value("server").toString();
    for (int i = 0; i < m_serverCombo->count(); ++i) {
        if (m_serverCombo->itemData(i).toString() == server) {
            m_serverCombo->setCurrentIndex(i);
            break;
        }
    }
    
    m_autoStartCheck->setChecked(m_settings->value("autoStart", false).toBool());
}

void ConfigDialog::saveConfig()
{
    m_settings->setValue("user", m_userEdit->text());
    m_settings->setValue("password", m_passwordEdit->text());
    m_settings->setValue("server", m_serverCombo->currentData().toString());
    m_settings->setValue("autoStart", m_autoStartCheck->isChecked());
    m_settings->sync();
}

void ConfigDialog::onSaveClicked()
{
    if (m_userEdit->text().isEmpty() || m_passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "警告", "请填写完整的学号和密码信息！");
        return;
    }
    
    saveConfig();
    accept();
}

void ConfigDialog::onCancelClicked()
{
    reject();
}

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
}

MainWindow::~MainWindow()
{
    delete ui;
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
    m_isFirstRun = !settings.contains("user");
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
    m_configDialog->loadConfig();
    if (m_configDialog->exec() == QDialog::Accepted) {
        setupAutoStart();
    }
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
    } else {
        m_trayIcon->showMessage("AUST WiFi", "登录失败: " + message, 
                               QSystemTrayIcon::Warning, 3000);
    }
}
