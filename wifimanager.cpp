#include "wifimanager.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QThread>
#include <QEventLoop>
#include <QDebug>

WiFiManager::WiFiManager(QObject *parent)
    : QObject(parent)
    , m_checkTimer(nullptr)
    , m_networkManager(nullptr)
    , m_currentReply(nullptr)
    , m_loginReply(nullptr)
    , m_isConnected(false)
    , m_settings(nullptr)
    , m_currentSSID("")
    , m_detectedUserType("student")
    , m_loginRetryCount(0)
    , m_isConfiguring(false)
    , m_lastLoginSuccess()
{
    // 初始化设置
    m_settings = new QSettings("AUST_WIFI", "Config", this);
    
    // 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);
    
    // 初始化定时器
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &WiFiManager::checkConnection);
    
    // 初始化当前SSID
    m_currentSSID = getCurrentWifiSSID();
    qDebug() << "程序启动时的WiFi SSID:" << m_currentSSID;
    
    // 添加定时器状态监控，每10秒检查一次定时器是否正常工作
    QTimer *watchdogTimer = new QTimer(this);
    connect(watchdogTimer, &QTimer::timeout, [this]() {
        if (m_checkTimer) {
            qDebug() << "定时器状态检查 - 是否活跃:" << m_checkTimer->isActive() 
                     << ", 间隔:" << m_checkTimer->interval() << "ms";
            
            // 检查网络管理器状态
            if (m_networkManager) {
                qDebug() << "网络管理器状态正常";
            } else {
                qDebug() << "警告：网络管理器对象为空";
            }
            
            // 检查当前请求状态
            if (m_currentReply) {
                qDebug() << "当前有网络检测请求在进行，状态:" << (m_currentReply->isRunning() ? "运行中" : "已完成");
            } else {
                qDebug() << "当前没有网络检测请求";
            }
            
            // 如果定时器意外停止，重新启动
            if (!m_checkTimer->isActive()) {
                qDebug() << "检测到定时器已停止，重新启动...";
                m_checkTimer->start(1000);
            }
        }
    });
    watchdogTimer->start(10000); // 每10秒检查一次，更频繁的监控
    
#ifdef USE_LIBCURL
    // 初始化CURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif
}

WiFiManager::~WiFiManager()
{
    // 停止所有活动
    stopAutoReconnect();
    
    // 清理网络请求
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    if (m_loginReply) {
        m_loginReply->abort();
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
    }
    
#ifdef USE_LIBCURL
    curl_global_cleanup();
#endif
}

void WiFiManager::saveConfig(const QString &user, const QString &password, const QString &server)
{
    m_settings->setValue("user", user);
    m_settings->setValue("password", password);
    m_settings->setValue("server", server);
    m_settings->sync();
}

bool WiFiManager::loadConfig(QString &user, QString &password, QString &server)
{
    if (!m_settings->contains("user")) {
        return false;
    }
    
    user = m_settings->value("user").toString();
    password = m_settings->value("password").toString();
    server = m_settings->value("server").toString();
    
    return !user.isEmpty() && !password.isEmpty() && !server.isEmpty();
}

bool WiFiManager::loadConfig(QString &user, QString &password, QString &server, QString &userType)
{
    if (!m_settings->contains("user")) {
        return false;
    }
    
    user = m_settings->value("user").toString();
    password = m_settings->value("password").toString();
    server = m_settings->value("server").toString();
    userType = m_settings->value("userType", "student").toString();
    
    return !user.isEmpty() && !password.isEmpty() && !server.isEmpty();
}

bool WiFiManager::hasConfig()
{
    return m_settings->contains("user") && 
           m_settings->contains("password") && 
           m_settings->contains("server");
}

// 双配置管理实现
void WiFiManager::saveStudentConfig(const QString &user, const QString &password, const QString &server)
{
    m_settings->setValue("student/user", user);
    m_settings->setValue("student/password", password);
    m_settings->setValue("student/server", server);
    m_settings->sync();
    qDebug() << "学生配置已保存：" << user << "@" << server;
}

void WiFiManager::saveTeacherConfig(const QString &user, const QString &password)
{
    m_settings->setValue("teacher/user", user);
    m_settings->setValue("teacher/password", password);
    m_settings->setValue("teacher/server", "jzg");  // 教师固定使用jzg
    m_settings->sync();
    qDebug() << "教师配置已保存：" << user << "@jzg";
}

bool WiFiManager::loadStudentConfig(QString &user, QString &password, QString &server)
{
    if (!hasStudentConfig()) {
        return false;
    }
    
    user = m_settings->value("student/user").toString();
    password = m_settings->value("student/password").toString();
    server = m_settings->value("student/server").toString();
    
    qDebug() << "加载学生配置：" << user << "@" << server;
    return !user.isEmpty() && !password.isEmpty() && !server.isEmpty();
}

bool WiFiManager::loadTeacherConfig(QString &user, QString &password)
{
    if (!hasTeacherConfig()) {
        return false;
    }
    
    user = m_settings->value("teacher/user").toString();
    password = m_settings->value("teacher/password").toString();
    
    qDebug() << "加载教师配置：" << user << "@jzg";
    return !user.isEmpty() && !password.isEmpty();
}

bool WiFiManager::hasStudentConfig()
{
    return m_settings->contains("student/user") && 
           m_settings->contains("student/password") && 
           m_settings->contains("student/server");
}

bool WiFiManager::hasTeacherConfig()
{
    return m_settings->contains("teacher/user") && 
           m_settings->contains("teacher/password");
}

bool WiFiManager::loadConfigByUserType(const QString &userType, QString &user, QString &password, QString &server)
{
    if (userType == "teacher") {
        if (loadTeacherConfig(user, password)) {
            server = "jzg";  // 教师固定使用jzg
            return true;
        }
    } else if (userType == "student") {
        return loadStudentConfig(user, password, server);
    }
    
    qDebug() << "未找到" << userType << "类型的配置";
    return false;
}

// 配置状态管理实现
void WiFiManager::pauseNetworkOperations()
{
    qDebug() << "暂停网络操作 - 用户正在修改配置";
    m_isConfiguring = true;
    
    // 停止定时器
    if (m_checkTimer && m_checkTimer->isActive()) {
        m_checkTimer->stop();
        qDebug() << "已停止网络检测定时器";
    } else {
        qDebug() << "定时器未运行，无需停止";
    }
    
    // 取消正在进行的网络请求
    if (m_currentReply && m_currentReply->isRunning()) {
        m_currentReply->abort();
        qDebug() << "已取消网络检测请求";
    } else {
        qDebug() << "无正在进行的网络检测请求";
    }
    
    // 取消并清理正在进行的登录请求
    if (m_loginReply) {
        if (m_loginReply->isRunning()) {
            m_loginReply->abort();
            qDebug() << "已取消登录请求";
        }
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
        qDebug() << "已清理登录请求对象";
    } else {
        qDebug() << "无正在进行的登录请求";
    }
    
    qDebug() << "网络操作暂停完成，配置状态已设置为true";
}

void WiFiManager::resumeNetworkOperations()
{
    qDebug() << "恢复网络操作 - 配置修改完成";
    m_isConfiguring = false;
    
    // 重新启动网络检测
    if (m_checkTimer) {
        if (!m_checkTimer->isActive()) {
            m_checkTimer->start(1000);  // 1秒后开始检测
            qDebug() << "已重新启动网络检测定时器";
        } else {
            qDebug() << "定时器已在运行，无需重新启动";
        }
    } else {
        qDebug() << "错误：定时器对象为空，无法启动网络检测";
    }
    
    qDebug() << "网络操作恢复完成，配置状态已设置为false";
}

bool WiFiManager::isConfiguring() const
{
    return m_isConfiguring;
}

// 配置验证实现
QString WiFiManager::validateConfiguration(const QString &userType)
{
    QString user, password, server;
    
    if (userType == "teacher") {
        if (!hasTeacherConfig()) {
            return "未配置教师账号信息，请在配置界面中设置工号和密码";
        }
        
        if (!loadTeacherConfig(user, password)) {
            return "教师配置加载失败，请检查配置信息";
        }
        
        if (user.isEmpty() || password.isEmpty()) {
            return "教师配置不完整，工号或密码为空";
        }
        
        if (user.length() > 8) {
            return "工号格式不正确，教师工号应为8位以下";
        }
        
    } else if (userType == "student") {
        if (!hasStudentConfig()) {
            return "未配置学生账号信息，请在配置界面中设置学号、密码和运营商";
        }
        
        if (!loadStudentConfig(user, password, server)) {
            return "学生配置加载失败，请检查配置信息";
        }
        
        if (user.isEmpty() || password.isEmpty() || server.isEmpty()) {
            return "学生配置不完整，学号、密码或运营商为空";
        }
        
        if (user.length() < 8) {
            return "学号格式不正确，请输入完整学号";
        }
        
    } else {
        return "用户类型无效：" + userType;
    }
    
    return "";  // 配置有效
}

bool WiFiManager::hasValidConfiguration()
{
    bool hasValidStudent = hasStudentConfig() && validateConfiguration("student").isEmpty();
    bool hasValidTeacher = hasTeacherConfig() && validateConfiguration("teacher").isEmpty();
    
    return hasValidStudent || hasValidTeacher;
}

// 状态检查实现
void WiFiManager::printCurrentStatus()
{
    qDebug() << "=== WiFiManager 当前状态 ===";
    qDebug() << "- 是否正在配置:" << m_isConfiguring;
    qDebug() << "- 定时器对象:" << (m_checkTimer ? "存在" : "为空");
    qDebug() << "- 定时器是否活动:" << (m_checkTimer ? m_checkTimer->isActive() : false);
    qDebug() << "- 当前网络检测请求:" << (m_currentReply ? "存在" : "为空");
    qDebug() << "- 当前登录请求:" << (m_loginReply ? "存在" : "为空");
    qDebug() << "- 网络连接状态:" << m_isConnected;
    qDebug() << "- 当前WiFi SSID:" << m_currentSSID;
    qDebug() << "- 检测到的用户类型:" << m_detectedUserType;
    qDebug() << "- 学生配置:" << (hasStudentConfig() ? "已配置" : "未配置");
    qDebug() << "- 教师配置:" << (hasTeacherConfig() ? "已配置" : "未配置");
    qDebug() << "========================";
}

bool WiFiManager::isTimerActive() const
{
    return m_checkTimer && m_checkTimer->isActive();
}

void WiFiManager::checkInternetConnection()
{
    // 如果有登录请求正在进行，跳过网络检测避免冲突
    static int skipCount = 0;
    
    if (m_loginReply && m_loginReply->isRunning()) {
        qDebug() << "登录请求进行中，跳过网络检测";
        
        // 防止登录请求卡死，添加保护机制
        skipCount++;
        if (skipCount > 15) {  // 15秒后强制清理，与登录超时时间匹配
            qDebug() << "登录请求超过15秒未完成，可能卡住，强制清理";
            if (m_loginReply) {
                m_loginReply->abort();
                m_loginReply->deleteLater();
                m_loginReply = nullptr;
                qDebug() << "已清理卡住的登录请求";
            }
            skipCount = 0;
        }
        return;
    } else {
        // 没有登录请求时重置计数器
        skipCount = 0;
    }
    
    // 检查是否有未完成的网络检测请求
    if (m_currentReply) {
        if (m_currentReply->isRunning()) {
            qDebug() << "上一个网络检测请求仍在进行中，跳过本次检测";
            // 不强制取消，让上一个请求自然完成
            return;
        }
        // 只清理已完成的请求
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    qDebug() << "发送网络检测请求到: http://www.baidu.com";
    
    // 恢复外网连通性检测，这是用户的需求
    QNetworkRequest request(QUrl("http://www.baidu.com"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    
    // 设置合理的超时时间
    request.setTransferTimeout(8000);  // 8秒超时，平衡速度与稳定性
    
    // 使用HEAD请求检测外网连通性
    m_currentReply = m_networkManager->head(request);
    
    if (m_currentReply) {
        qDebug() << "网络请求对象创建成功，连接信号...";
        connect(m_currentReply, &QNetworkReply::finished, this, &WiFiManager::onConnectionCheckFinished);
        
        // 使用更温和的超时处理，不强制abort
        QPointer<WiFiManager> self(this);
        QTimer::singleShot(6000, [self]() {
            if (!self) {
                return; // 对象已销毁
            }
            if (self->m_currentReply && self->m_currentReply->isRunning()) {
                qDebug() << "网络检测请求超过6秒，可能网络较慢，继续等待...";
                // 不强制取消，只记录状态，让Qt的超时机制处理
            }
        });
    } else {
        qDebug() << "错误：网络请求对象创建失败";
    }
}

void WiFiManager::sendLoginRequest(const QString &user, const QString &password, const QString &server)
{
#ifdef USE_LIBCURL
    sendPostRequest(user, password, server);
#else
    sendPostRequestQt(user, password, server);
#endif
}

void WiFiManager::startAutoReconnect()
{
    qDebug() << "启动自动重连...";
    if (m_checkTimer && !m_checkTimer->isActive()) {
        qDebug() << "启动检测定时器，间隔1秒";
        m_checkTimer->start(1000); // 每秒检查一次
    } else if (m_checkTimer && m_checkTimer->isActive()) {
        qDebug() << "检测定时器已经在运行";
    } else {
        qDebug() << "错误：检测定时器对象为空";
    }
}

void WiFiManager::stopAutoReconnect()
{
    qDebug() << "停止自动重连...";
    if (m_checkTimer && m_checkTimer->isActive()) {
        qDebug() << "停止检测定时器";
        m_checkTimer->stop();
    }
    
    // 清理正在进行的网络请求
    if (m_currentReply && m_currentReply->isRunning()) {
        qDebug() << "终止网络检测请求";
        m_currentReply->abort();
    }
    if (m_loginReply && m_loginReply->isRunning()) {
        qDebug() << "终止登录请求";
        m_loginReply->abort();
    }
}

void WiFiManager::checkConnection()
{
    // 如果正在配置，跳过所有网络操作
    if (m_isConfiguring) {
        qDebug() << "跳过网络检测 - 用户正在修改配置";
        return;
    }
    
    qDebug() << "定时器触发，开始检测网络连接...";
    checkInternetConnection();
}

void WiFiManager::onConnectionCheckFinished()
{
    bool connected = false;
    QString newSSID = "";
    
    qDebug() << "开始处理网络检测结果...";
    
    // 检查是否正在配置中，如果是则跳过处理
    if (m_isConfiguring) {
        qDebug() << "用户正在配置中，跳过网络检测结果处理";
        if (m_currentReply) {
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
        }
        return;
    }
    
    // 无论m_currentReply是否存在，都要检测WiFi变化
    newSSID = getCurrentWifiSSID();
    qDebug() << "检测到当前WiFi SSID:" << newSSID;
    
    bool ssidChanged = (newSSID != m_currentSSID && !m_currentSSID.isEmpty());
    if (ssidChanged) {
        qDebug() << "检测到WiFi网络变化：" << m_currentSSID << " -> " << newSSID;
        m_currentSSID = newSSID;
        // WiFi改变时强制重新认证
        connected = false;
        qDebug() << "WiFi变化导致需要重新认证";
    } else {
        qDebug() << "WiFi网络无变化，当前SSID:" << m_currentSSID;
    }
    
    if (m_currentReply) {
        QNetworkReply::NetworkError error = m_currentReply->error();
        qDebug() << "网络请求错误代码:" << error;
        
        if (!ssidChanged) {  // 只有在WiFi没有变化时才检查网络状态
            if (error == QNetworkReply::NoError) {
                // 外网连接正常
                QVariant statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                qDebug() << "HTTP状态码:" << statusCode.toInt();
                if (statusCode.isValid() && statusCode.toInt() == 200) {
                    connected = true;
                    qDebug() << "网络连接正常，HTTP状态码: 200";
                } else {
                    qDebug() << "网络请求成功但状态码异常:" << statusCode.toInt();
                    connected = false;
                }
            } else {
                qDebug() << "网络请求失败:" << m_currentReply->errorString();
                connected = false;
            }
        }
        
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        qDebug() << "网络请求对象已清理";
    } else {
        qDebug() << "网络检测回复对象为空（可能已被清理或回调重复调用）";
        if (!ssidChanged) {
            // 没有WiFi变化且没有检测结果，保持当前状态不变
            qDebug() << "保持当前连接状态不变:" << m_isConnected;
            return;  // 直接返回，不触发状态变化
        } else {
            connected = false;  // WiFi变化时仍需要重新认证
        }
    }
    
    qDebug() << "检测结果：connected =" << connected << ", m_isConnected =" << m_isConnected;
    
    if (connected != m_isConnected) {
        qDebug() << "连接状态发生变化：" << m_isConnected << " -> " << connected;
        m_isConnected = connected;
        emit connectionStatusChanged(connected);
    }
    
    if (!connected) {
        qDebug() << "网络未连通，检查是否需要登录...";
        
        // 检查是否在登录成功后的容错期内
        if (m_lastLoginSuccess.isValid()) {
            qint64 timeSinceLogin = m_lastLoginSuccess.msecsTo(QDateTime::currentDateTime());
            if (timeSinceLogin < LOGIN_GRACE_PERIOD_MS) {
                qDebug() << "刚刚登录成功" << timeSinceLogin/1000 << "秒，在容错期内，跳过重新登录";
                qDebug() << "网络可能正在稳定中，等待下次检测";
                return;  // 在容错期内，不进行重新登录
            } else {
                qDebug() << "超出登录成功容错期(" << LOGIN_GRACE_PERIOD_MS/1000 << "秒)，允许重新登录";
            }
        }
        
        // 只有在没有正在进行的登录请求时才尝试重新连接
        if (!m_loginReply || !m_loginReply->isRunning()) {
            // 检测当前WiFi SSID并确定用户类型
            QString currentSSID = getCurrentWifiSSID();
            QString autoUserType = determineUserTypeBySSID(currentSSID);
            
            // 更新成员变量
            m_currentSSID = currentSSID;
            m_detectedUserType = autoUserType;
            
            qDebug() << "网络未连通，尝试登录...";
            qDebug() << "当前WiFi SSID:" << currentSSID;
            qDebug() << "根据SSID检测的用户类型:" << autoUserType;
            
            // 使用SSID检测的用户类型，直接加载对应配置
            QString effectiveUserType = autoUserType;
            qDebug() << "最终使用的用户类型:" << effectiveUserType;
            
            // 先验证配置有效性
            QString configError = validateConfiguration(effectiveUserType);
            if (!configError.isEmpty()) {
                qDebug() << "配置验证失败:" << configError;
                emit loginResult(false, configError);
                return;
            }
            
            QString user, password, server;
            if (loadConfigByUserType(effectiveUserType, user, password, server)) {
                qDebug() << "使用账号:" << user << "进行" << (effectiveUserType == "teacher" ? "教师" : "学生") << "登录";
                
                // 开始新的登录尝试时重置重试计数器
                m_loginRetryCount = 0;
                
                if (effectiveUserType == "teacher") {
                    // 教师使用专门的GET请求登录方法
                    qDebug() << "教师使用GET请求登录到drcom端点";
#ifdef USE_LIBCURL
                    sendTeacherLoginRequest(user, password);
#else
                    sendTeacherLoginRequestQt(user, password);
#endif
                } else {
                    // 学生使用POST请求方法
                    sendLoginRequest(user, password, server);
                }
            } else {
                qDebug() << "配置加载失败，无法进行登录";
                emit loginResult(false, QString("配置加载失败，请重新配置%1账号信息")
                                 .arg(effectiveUserType == "teacher" ? "教师" : "学生"));
            }
        } else {
            qDebug() << "登录请求正在进行中，跳过...";
        }
    } else {
        qDebug() << "网络连接正常，无需登录";
        // 网络连接正常时，确保连接状态为true
        if (!m_isConnected) {
            qDebug() << "更新内部连接状态为true";
            m_isConnected = true;
            emit connectionStatusChanged(true);
        }
    }
}

void WiFiManager::onLoginFinished()
{
    if (!m_loginReply) {
        qDebug() << "登录完成回调：登录请求对象为空";
        return;
    }
    
    QNetworkReply::NetworkError error = m_loginReply->error();
    QString response;
    
    // 安全地读取响应内容
    if (m_loginReply->isOpen() && m_loginReply->isReadable()) {
        response = m_loginReply->readAll();
    } else {
        qDebug() << "登录响应无法读取，设备状态:" << m_loginReply->isOpen() << m_loginReply->isReadable();
    }
    
    qDebug() << "登录请求完成，错误代码:" << error << "响应长度:" << response.length();
    
    if (error == QNetworkReply::NoError) {
            qDebug() << "登录响应:" << response;
            
            // 检查响应内容判断登录是否成功
            bool loginSuccess = false;
            if (response.contains("\"result\":1")) {
                // 教师登录成功标志（JSONP响应）
                loginSuccess = true;
                qDebug() << "检测到教师登录成功标志: result:1";
            } else if (response.contains("login_ok") || response.isEmpty()) {
                // 学生登录成功标志（根据实际情况调整）
                loginSuccess = true;
                qDebug() << "检测到学生登录成功";
            } else if (response.contains("UID=") && response.contains("@jzg") && response.contains("ac0=")) {
                // 教师登录成功标志（HTML页面包含用户信息）
                loginSuccess = true;
                qDebug() << "检测到教师登录成功标志: HTML页面包含用户配置信息";
            } else if (response.length() > 1000 && response.contains("<script") && response.contains("UID=")) {
                // 教师登录可能成功（返回了配置页面）
                loginSuccess = true;
                qDebug() << "检测到可能的教师登录成功: 返回了包含用户信息的配置页面";
            } else if (response.contains("\"uid\":") && response.contains("@jzg") && 
                      (response.contains("\"ss5\":") || response.contains("\"ss6\":"))) {
                // 教师登录成功标志（JSONP响应包含用户详细信息）
                // 即使result:0，但包含uid和服务器信息说明认证成功
                loginSuccess = true;
                qDebug() << "检测到教师登录成功标志: JSONP响应包含用户信息 (uid, ss5/ss6)";
            }
            
            if (loginSuccess) {
                emit loginResult(true, "登录成功");
                // 重置重试计数器
                m_loginRetryCount = 0;
                // 登录成功后，设置连接状态为true，避免立即重新登录
                m_isConnected = true;
                emit connectionStatusChanged(true);
                qDebug() << "登录成功，更新连接状态为已连接";
                
                // 记录登录成功时间，用于后续容错判断
                m_lastLoginSuccess = QDateTime::currentDateTime();
                qDebug() << "记录登录成功时间:" << m_lastLoginSuccess.toString();
                
                // 延迟3秒后再次检测网络，更快响应网络状态变化
                // 教师网络登录后可能需要额外的认证步骤
                QPointer<WiFiManager> self(this);
                QTimer::singleShot(3000, [self]() {
                    if (!self) return;
                    qDebug() << "登录成功后延迟检测网络连通性";
                    self->checkInternetConnection();
                });
            } else {
                qDebug() << "登录响应不包含成功标志，可能登录失败";
                handleLoginFailure("服务器响应异常");
                return; // 失败情况下直接返回，避免重复清理
            }
        } else {
            qDebug() << "登录请求失败:" << m_loginReply->errorString();
            handleLoginFailure(m_loginReply->errorString());
            // handleLoginFailure已经清理了m_loginReply，这里不需要重复清理
            return;
        }
        
        // 只有成功的情况才在这里清理
        if (m_loginReply) {
            m_loginReply->deleteLater();
            m_loginReply = nullptr;
        }
    }

#ifdef USE_LIBCURL
size_t WiFiManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((QString*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void WiFiManager::sendPostRequest(const QString &user, const QString &password, const QString &server)
{
    CURL* curl;
    CURLcode res;
    long http_code = 0;
    
    QString data = QString("callback=dr1003&DDDDD=%1@%2&upass=%3&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=")
                   .arg(user)
                   .arg(server)
                   .arg(password);
    
    qDebug() << "发送的数据:" << data;
    
    QString response;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.255.0.19/a79.htm");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.toUtf8().constData());
        
        // 设置Content-Type头
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // 设置接收响应的回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (res != CURLE_OK) {
            qDebug() << "curl_easy_perform() failed:" << curl_easy_strerror(res);
            emit loginResult(false, QString("CURL错误: %1").arg(curl_easy_strerror(res)));
        } else {
            qDebug() << "HTTP状态码:" << http_code;
            emit loginResult(true, QString("请求已发送，状态码: %1").arg(http_code));
        }
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        qDebug() << "初始化curl失败！";
        emit loginResult(false, "初始化CURL失败");
    }
}

void WiFiManager::sendTeacherLoginRequest(const QString &user, const QString &password)
{
    CURL* curl;
    CURLcode res;
    long http_code = 0;
    
    // 教师登录使用GET请求到drcom端点
    QString urlWithParams = QString("http://10.255.0.19/drcom/login?callback=dr1003&DDDDD=%1@jzg&upass=%2&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=")
                            .arg(user)
                            .arg(password);
    
    qDebug() << "教师登录GET请求URL:" << urlWithParams;
    
    QString response;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, urlWithParams.toUtf8().constData());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);  // 20秒超时，避免过早取消
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (res != CURLE_OK) {
            qDebug() << "教师登录CURL错误:" << curl_easy_strerror(res);
            emit loginResult(false, QString("教师登录网络错误: %1").arg(curl_easy_strerror(res)));
        } else {
            qDebug() << "教师登录HTTP状态码:" << http_code << "响应长度:" << response.length();
            qDebug() << "教师登录响应内容:" << response.left(200);
            
            // 检查教师登录成功标志 - 根据实际响应内容调整
            if (response.contains("\"result\":1") || response.contains("login_ok") || 
                (http_code == 200 && !response.contains("login") && !response.contains("error"))) {
                emit loginResult(true, "教师登录成功");
            } else {
                emit loginResult(false, "教师登录失败：服务器响应异常");
            }
        }
        
        curl_easy_cleanup(curl);
    } else {
        emit loginResult(false, "教师登录：初始化CURL失败");
    }
}

#else
void WiFiManager::sendPostRequestQt(const QString &user, const QString &password, const QString &server)
{
    QNetworkRequest request(QUrl("http://10.255.0.19/a79.htm"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setTransferTimeout(10000); // 10秒超时，更快响应登录问题
    
    QString data = QString("callback=dr1003&DDDDD=%1@%2&upass=%3&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=")
                   .arg(user)
                   .arg(server)
                   .arg(password);
    
    qDebug() << "发送的数据:" << data;
    
    // 清理之前的登录请求
    if (m_loginReply) {
        qDebug() << "清理之前的登录请求";
        m_loginReply->abort();
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
    }
    
    qDebug() << "发送POST登录请求到:" << request.url().toString();
    m_loginReply = m_networkManager->post(request, data.toUtf8());
    
    if (m_loginReply) {
        connect(m_loginReply, &QNetworkReply::finished, this, &WiFiManager::onLoginFinished);
        
        // 添加错误处理
        connect(m_loginReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
                [this](QNetworkReply::NetworkError error) {
                    qDebug() << "登录请求网络错误:" << error << (m_loginReply ? m_loginReply->errorString() : "未知错误");
                });
        
        qDebug() << "登录请求已发送，等待响应...";
    } else {
        qDebug() << "错误：登录请求对象创建失败";
        emit loginResult(false, "创建登录请求失败");
    }
}

void WiFiManager::sendTeacherLoginRequestQt(const QString &user, const QString &password)
{
    // 教师登录使用GET请求到drcom端点
    QString urlWithParams = QString("http://10.255.0.19/drcom/login?callback=dr1003&DDDDD=%1@jzg&upass=%2&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=")
                            .arg(user)
                            .arg(password);
    
    QUrl url(urlWithParams);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setTransferTimeout(12000); // 12秒超时，更快响应登录问题
    
    // 添加更多HTTP头，提高兼容性
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
    request.setRawHeader("Connection", "keep-alive");
    
    qDebug() << "教师登录GET请求URL:" << urlWithParams;
    
    // 清理之前的登录请求
    if (m_loginReply) {
        qDebug() << "清理之前的教师登录请求";
        m_loginReply->abort();
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
    }
    
    qDebug() << "发送GET教师登录请求到:" << request.url().toString();
    m_loginReply = m_networkManager->get(request);  // 使用GET方法
    
    if (m_loginReply) {
        connect(m_loginReply, &QNetworkReply::finished, this, &WiFiManager::onLoginFinished);
        
        // 添加错误处理
        connect(m_loginReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
                [this](QNetworkReply::NetworkError error) {
                    qDebug() << "教师登录请求网络错误:" << error << (m_loginReply ? m_loginReply->errorString() : "未知错误");
                });
        
        qDebug() << "教师登录请求已发送，等待响应...";
    } else {
        qDebug() << "错误：教师登录请求对象创建失败";
        emit loginResult(false, "创建教师登录请求失败");
    }
}

#endif 

QString WiFiManager::getCurrentWifiSSID()
{
    QProcess process;
    process.start("netsh", QStringList() << "wlan" << "show" << "profiles");
    process.waitForFinished(3000); // 等待3秒
    
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << "获取WiFi配置文件失败:" << process.errorString();
        return "";
    }
    
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QStringList lines = output.split('\n');
    
    // 查找当前连接的WiFi
    QProcess currentProcess;
    currentProcess.start("netsh", QStringList() << "wlan" << "show" << "interfaces");
    currentProcess.waitForFinished(3000);
    
    if (currentProcess.exitStatus() != QProcess::NormalExit || currentProcess.exitCode() != 0) {
        qDebug() << "获取当前WiFi接口失败:" << currentProcess.errorString();
        return "";
    }
    
    QString interfaceOutput = QString::fromLocal8Bit(currentProcess.readAllStandardOutput());
    QStringList interfaceLines = interfaceOutput.split('\n');
    
    for (const QString &line : interfaceLines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.contains("SSID") && trimmedLine.contains(":")) {
            QStringList parts = trimmedLine.split(":");
            if (parts.size() >= 2) {
                QString ssid = parts[1].trimmed();
                if (!ssid.isEmpty()) {
                    qDebug() << "检测到当前WiFi SSID:" << ssid;
                    return ssid;
                }
            }
        }
    }
    
    qDebug() << "未能检测到当前WiFi SSID";
    return "";
}

QString WiFiManager::determineUserTypeBySSID(const QString &ssid)
{
    if (ssid.isEmpty()) {
        qDebug() << "SSID为空，默认使用学生登录";
        return "student";
    }
    
    // 根据WiFi名称确定用户类型
    if (ssid == "AUST_Student") {
        qDebug() << "检测到学生WiFi网络，使用学生登录";
        return "student";
    } else if (ssid == "AUST_Faculty") {
        qDebug() << "检测到教师WiFi网络，使用教师登录";
        return "teacher";
    } else {
        qDebug() << "检测到其他WiFi网络:" << ssid << "，默认使用学生登录";
        return "student";
    }
} 

void WiFiManager::handleLoginFailure(const QString &errorMessage)
{
    m_loginRetryCount++;
    qDebug() << "登录失败，重试次数:" << m_loginRetryCount << "/" << MAX_LOGIN_RETRIES;
    qDebug() << "失败原因:" << errorMessage;
    
    // 确保登录请求状态被清理
    if (m_loginReply) {
        qDebug() << "清理失败的登录请求对象";
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
    }
    
    if (m_loginRetryCount < MAX_LOGIN_RETRIES) {
        // 延迟重试，避免过于频繁的请求
        int retryDelay = m_loginRetryCount * 3000; // 3秒, 6秒, 9秒递增延迟
        qDebug() << "将在" << retryDelay/1000 << "秒后重试登录";
        
        // 使用 QPointer 确保对象生命周期安全
        QPointer<WiFiManager> self(this);
        QTimer::singleShot(retryDelay, [self]() {
            if (!self) {
                qDebug() << "WiFiManager 对象已销毁，取消重试";
                return;
            }
            
            qDebug() << "开始重试登录，检查当前状态...";
            qDebug() << "- 登录请求对象:" << (self->m_loginReply ? "存在" : "为空");
            qDebug() << "- 当前连接状态:" << self->m_isConnected;
            
            // 重新触发登录流程
            if (!self->m_isConnected && !self->m_loginReply) {
                self->checkConnection();
            } else {
                qDebug() << "跳过重试 - 已连接或有登录请求在进行";
            }
        });
    } else {
        // 达到最大重试次数，报告失败并重置计数器
        qDebug() << "达到最大重试次数，停止重试";
        m_loginRetryCount = 0;
        emit loginResult(false, QString("登录失败 (已重试%1次): %2").arg(MAX_LOGIN_RETRIES).arg(errorMessage));
        
        // 延迟较长时间后再次尝试
        QPointer<WiFiManager> self(this);
        QTimer::singleShot(30000, [self]() {  // 30秒后重新开始
            if (!self) {
                qDebug() << "WiFiManager 对象已销毁，取消30秒冷却重试";
                return;
            }
            
            qDebug() << "30秒冷却时间结束，重新开始检测";
            if (!self->m_isConnected && !self->m_loginReply) {
                self->checkConnection();
            }
        });
    }
}