#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QDebug>
#include <QProcess>
#include <QPointer>
#include <QDateTime>

#ifdef USE_LIBCURL
#include <curl/curl.h>
#endif

class WiFiManager : public QObject
{
    Q_OBJECT

public:
    explicit WiFiManager(QObject *parent = nullptr);
    ~WiFiManager();

    // 配置相关
    void saveConfig(const QString &user, const QString &password, const QString &server);
    bool loadConfig(QString &user, QString &password, QString &server);
    bool loadConfig(QString &user, QString &password, QString &server, QString &userType);
    bool hasConfig();
    
    // 双配置管理
    void saveStudentConfig(const QString &user, const QString &password, const QString &server);
    void saveTeacherConfig(const QString &user, const QString &password);
    bool loadStudentConfig(QString &user, QString &password, QString &server);
    bool loadTeacherConfig(QString &user, QString &password);
    bool hasStudentConfig();
    bool hasTeacherConfig();
    bool loadConfigByUserType(const QString &userType, QString &user, QString &password, QString &server);

    // 网络连接检测
    void checkInternetConnection();
    
    // WiFi SSID检测
    QString getCurrentWifiSSID();
    QString determineUserTypeBySSID(const QString &ssid);
    
    // 发送登录请求
    void sendLoginRequest(const QString &user, const QString &password, const QString &server);

    // 启动/停止自动重连
    void startAutoReconnect();
    void stopAutoReconnect();
    
    // 配置状态管理
    void pauseNetworkOperations();   // 暂停网络操作（配置界面打开时）
    void resumeNetworkOperations();  // 恢复网络操作（配置界面关闭时）
    bool isConfiguring() const;      // 是否正在配置中
    
    // 配置验证
    QString validateConfiguration(const QString &userType);  // 验证配置有效性
    bool hasValidConfiguration();    // 是否有有效配置
    
    // 状态检查
    void printCurrentStatus();      // 打印当前状态信息（用于调试）
    bool isTimerActive() const;     // 检查定时器是否活动

signals:
    void connectionStatusChanged(bool connected);
    void loginResult(bool success, const QString &message);

private slots:
    void checkConnection();
    void onConnectionCheckFinished();
    void onLoginFinished();
    void handleLoginFailure(const QString &errorMessage);

private:
    QTimer *m_checkTimer;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;        // 用于网络检测
    QNetworkReply *m_loginReply;          // 用于登录请求
    bool m_isConnected;
    
    // 配置相关
    QSettings *m_settings;
    
    // WiFi检测相关
    QString m_currentSSID;
    QString m_detectedUserType;
    
    // 重试机制
    int m_loginRetryCount;
    static const int MAX_LOGIN_RETRIES = 3;
    
    // 配置状态
    bool m_isConfiguring;  // 是否正在配置中
    
    // 登录成功后的容错期
    QDateTime m_lastLoginSuccess;  // 上次登录成功时间
    static const int LOGIN_GRACE_PERIOD_MS = 15000;  // 15秒容错期，更快响应断网
    
    // 网络请求相关
#ifdef USE_LIBCURL
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void sendPostRequest(const QString &user, const QString &password, const QString &server);
    void sendTeacherLoginRequest(const QString &user, const QString &password);
#else
    void sendPostRequestQt(const QString &user, const QString &password, const QString &server);
    void sendTeacherLoginRequestQt(const QString &user, const QString &password);
#endif
};

#endif // WIFIMANAGER_H 