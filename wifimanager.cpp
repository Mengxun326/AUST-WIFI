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
{
    // 初始化设置
    m_settings = new QSettings("AUST_WIFI", "Config", this);
    
    // 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);
    
    // 初始化定时器
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &WiFiManager::checkConnection);
    
#ifdef USE_LIBCURL
    // 初始化CURL
    curl_global_init(CURL_GLOBAL_ALL);
#endif
}

WiFiManager::~WiFiManager()
{
    stopAutoReconnect();
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

bool WiFiManager::hasConfig()
{
    return m_settings->contains("user") && 
           m_settings->contains("password") && 
           m_settings->contains("server");
}

void WiFiManager::checkInternetConnection()
{
    QNetworkRequest request(QUrl("http://www.baidu.com"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    
    // 设置超时（2秒，与原始代码保持一致）
    request.setTransferTimeout(2000);
    
    if (m_currentReply) {
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    // 使用HEAD请求，只获取头部信息（与原始代码的CURLOPT_NOBODY一致）
    m_currentReply = m_networkManager->head(request);
    
    // 备用超时机制
    QTimer::singleShot(2500, [this]() {
        if (m_currentReply && m_currentReply->isRunning()) {
            m_currentReply->abort();
        }
    });
    
    connect(m_currentReply, &QNetworkReply::finished, this, &WiFiManager::onConnectionCheckFinished);
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
    if (!m_checkTimer->isActive()) {
        m_checkTimer->start(1000); // 每秒检查一次
    }
}

void WiFiManager::stopAutoReconnect()
{
    if (m_checkTimer->isActive()) {
        m_checkTimer->stop();
    }
    
    // 清理正在进行的网络请求
    if (m_currentReply && m_currentReply->isRunning()) {
        m_currentReply->abort();
    }
    if (m_loginReply && m_loginReply->isRunning()) {
        m_loginReply->abort();
    }
}

void WiFiManager::checkConnection()
{
    checkInternetConnection();
}

void WiFiManager::onConnectionCheckFinished()
{
    bool connected = false;
    
    if (m_currentReply) {
        // 检查网络错误和HTTP状态码，确保状态码为200（与原始代码保持一致）
        if (m_currentReply->error() == QNetworkReply::NoError) {
            QVariant statusCode = m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            if (statusCode.isValid() && statusCode.toInt() == 200) {
                connected = true;
                qDebug() << "网络连接正常，HTTP状态码: 200";
            } else {
                qDebug() << "网络请求成功但状态码异常:" << statusCode.toInt();
            }
        } else {
            qDebug() << "网络请求失败:" << m_currentReply->errorString();
        }
        
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    
    if (connected != m_isConnected) {
        m_isConnected = connected;
        emit connectionStatusChanged(connected);
    }
    
    if (!connected) {
        // 只有在没有正在进行的登录请求时才尝试重新连接
        if (!m_loginReply || !m_loginReply->isRunning()) {
            QString user, password, server;
            if (loadConfig(user, password, server)) {
                qDebug() << "网络未连通，尝试登录...";
                sendLoginRequest(user, password, server);
            }
        } else {
            qDebug() << "登录请求正在进行中，跳过...";
        }
    }
}

void WiFiManager::onLoginFinished()
{
    if (m_loginReply) {
        QNetworkReply::NetworkError error = m_loginReply->error();
        QString response = m_loginReply->readAll();
        
        if (error == QNetworkReply::NoError) {
            qDebug() << "登录响应:" << response;
            emit loginResult(true, "登录成功");
            // 登录后立即重新检测网络连接
            QTimer::singleShot(1000, this, &WiFiManager::checkInternetConnection);
        } else {
            qDebug() << "登录失败:" << m_loginReply->errorString();
            emit loginResult(false, "登录失败: " + m_loginReply->errorString());
        }
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
#else
void WiFiManager::sendPostRequestQt(const QString &user, const QString &password, const QString &server)
{
    QNetworkRequest request(QUrl("http://10.255.0.19/a79.htm"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    
    QString data = QString("callback=dr1003&DDDDD=%1@%2&upass=%3&0MKKey=123456&R1=0&R3=0&R6=0&para=00&v6ip=&v=")
                   .arg(user)
                   .arg(server)
                   .arg(password);
    
    qDebug() << "发送的数据:" << data;
    
    // 清理之前的登录请求
    if (m_loginReply) {
        m_loginReply->deleteLater();
        m_loginReply = nullptr;
    }
    
    m_loginReply = m_networkManager->post(request, data.toUtf8());
    connect(m_loginReply, &QNetworkReply::finished, this, &WiFiManager::onLoginFinished);
    
    // 设置超时
    QTimer::singleShot(10000, [this]() {
        if (m_loginReply && m_loginReply->isRunning()) {
            m_loginReply->abort();
            emit loginResult(false, "请求超时");
        }
    });
}
#endif 