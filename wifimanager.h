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
    bool hasConfig();

    // 网络连接检测
    void checkInternetConnection();
    
    // 发送登录请求
    void sendLoginRequest(const QString &user, const QString &password, const QString &server);

    // 启动/停止自动重连
    void startAutoReconnect();
    void stopAutoReconnect();

signals:
    void connectionStatusChanged(bool connected);
    void loginResult(bool success, const QString &message);

private slots:
    void checkConnection();
    void onConnectionCheckFinished();
    void onLoginFinished();

private:
    QTimer *m_checkTimer;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;        // 用于网络检测
    QNetworkReply *m_loginReply;          // 用于登录请求
    bool m_isConnected;
    
    // 配置相关
    QSettings *m_settings;
    
    // 网络请求相关
#ifdef USE_LIBCURL
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void sendPostRequest(const QString &user, const QString &password, const QString &server);
#else
    void sendPostRequestQt(const QString &user, const QString &password, const QString &server);
#endif
};

#endif // WIFIMANAGER_H 