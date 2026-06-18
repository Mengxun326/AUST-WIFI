#ifndef MOBILEBACKEND_H
#define MOBILEBACKEND_H

#include <QDateTime>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>

class MobileBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString studentUser READ studentUser WRITE setStudentUser NOTIFY configChanged)
    Q_PROPERTY(QString studentPassword READ studentPassword WRITE setStudentPassword NOTIFY configChanged)
    Q_PROPERTY(QString studentServer READ studentServer WRITE setStudentServer NOTIFY configChanged)
    Q_PROPERTY(QString teacherUser READ teacherUser WRITE setTeacherUser NOTIFY configChanged)
    Q_PROPERTY(QString teacherPassword READ teacherPassword WRITE setTeacherPassword NOTIFY configChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString activeAccountText READ activeAccountText NOTIFY configChanged)
    Q_PROPERTY(QString credentialBackendText READ credentialBackendText NOTIFY configChanged)
    Q_PROPERTY(QString networkStatusText READ networkStatusText NOTIFY networkStateChanged)
    Q_PROPERTY(QString currentSsid READ currentSsid NOTIFY networkStateChanged)
    Q_PROPERTY(bool autoLoginOnLaunch READ autoLoginOnLaunch WRITE setAutoLoginOnLaunch NOTIFY configChanged)
    Q_PROPERTY(bool autoLoginOnlyOnCampusWifi READ autoLoginOnlyOnCampusWifi WRITE setAutoLoginOnlyOnCampusWifi NOTIFY configChanged)
    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY networkStateChanged)
    Q_PROPERTY(bool campusWifiDetected READ campusWifiDetected NOTIFY networkStateChanged)
    Q_PROPERTY(bool campusGatewayReachable READ campusGatewayReachable NOTIFY networkStateChanged)
    Q_PROPERTY(bool backgroundServiceEnabled READ backgroundServiceEnabled WRITE setBackgroundServiceEnabled NOTIFY serviceStateChanged)
    Q_PROPERTY(bool notificationPermissionGranted READ notificationPermissionGranted NOTIFY serviceStateChanged)
    Q_PROPERTY(QString backgroundServiceStatusText READ backgroundServiceStatusText NOTIFY serviceStateChanged)
    Q_PROPERTY(QString updateStatusText READ updateStatusText NOTIFY updateStateChanged)
    Q_PROPERTY(QString updateVersionText READ updateVersionText NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateStateChanged)
    Q_PROPERTY(bool updateBusy READ updateBusy NOTIFY updateStateChanged)
    Q_PROPERTY(bool installPermissionGranted READ installPermissionGranted NOTIFY updateStateChanged)
    Q_PROPERTY(qreal updateDownloadProgress READ updateDownloadProgress NOTIFY updateStateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit MobileBackend(QObject *parent = nullptr);
    ~MobileBackend() override;

    QString studentUser() const;
    QString studentPassword() const;
    QString studentServer() const;
    QString teacherUser() const;
    QString teacherPassword() const;
    QString statusText() const;
    QString activeAccountText() const;
    QString credentialBackendText() const;
    QString networkStatusText() const;
    QString currentSsid() const;
    bool autoLoginOnLaunch() const;
    bool autoLoginOnlyOnCampusWifi() const;
    bool wifiConnected() const;
    bool campusWifiDetected() const;
    bool campusGatewayReachable() const;
    bool backgroundServiceEnabled() const;
    bool notificationPermissionGranted() const;
    QString backgroundServiceStatusText() const;
    QString updateStatusText() const;
    QString updateVersionText() const;
    bool updateAvailable() const;
    bool updateBusy() const;
    bool installPermissionGranted() const;
    qreal updateDownloadProgress() const;
    bool busy() const;

    void setStudentUser(const QString &value);
    void setStudentPassword(const QString &value);
    void setStudentServer(const QString &value);
    void setTeacherUser(const QString &value);
    void setTeacherPassword(const QString &value);
    void setAutoLoginOnLaunch(bool value);
    void setAutoLoginOnlyOnCampusWifi(bool value);
    void setBackgroundServiceEnabled(bool value);

    Q_INVOKABLE void loadConfig();
    Q_INVOKABLE bool saveConfig();
    Q_INVOKABLE void login();
    Q_INVOKABLE void cancelLogin();
    Q_INVOKABLE void refreshNetworkState();
    Q_INVOKABLE void requestNetworkPermissions();
    Q_INVOKABLE void requestNotificationPermission();
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void downloadAndInstallUpdate();
    Q_INVOKABLE void openInstallPermissionSettings();

signals:
    void configChanged();
    void statusChanged();
    void networkStateChanged();
    void serviceStateChanged();
    void updateStateChanged();
    void busyChanged();
    void loginSucceeded(const QString &message);
    void loginFailed(const QString &message);

private slots:
    void handleBackgroundServiceTick();
    void finishGatewayProbe();
    void finishUpdateCheck();
    void finishUpdateDownload();

private:
    enum class LoginMode {
        Student,
        Teacher
    };

    static QUrlQuery buildLoginQuery(const QString &user, const QString &server, const QString &password);
    void setStatusText(const QString &value);
    void setBusy(bool value);
    void startStudentLogin(const QString &user, const QString &password, const QString &server);
    void startTeacherLogin(const QString &user, const QString &password);
    bool hasUsableCredentials() const;
    void runStartupAutoLogin();
    void evaluateAutoLoginSchedule();
    void syncBackgroundServiceState();
    void updateNetworkRefreshTimer();
    void refreshNotificationPermission();
    void setBackgroundServiceStatusText(const QString &value);
    void refreshInstallPermission();
    void refreshCurrentAppVersion();
    void setUpdateStatusText(const QString &value);
    void setUpdateBusy(bool value);
    bool installDownloadedUpdate();
    void clearUpdateReplies();
    QString buildNetworkStatusText() const;
    void startGatewayProbe();
    void clearGatewayProbe();
    static bool isCampusWifiSsid(const QString &ssid);
    void finishLogin();
    bool responseLooksSuccessful(const QString &response) const;
    void clearReply();

    QSettings m_settings;
    QNetworkAccessManager m_networkManager;
    QNetworkAccessManager m_updateNetworkManager;
    QNetworkAccessManager m_gatewayProbeManager;
    QTimer m_networkRefreshTimer;
    QNetworkReply *m_reply = nullptr;
    QNetworkReply *m_updateManifestReply = nullptr;
    QNetworkReply *m_updateDownloadReply = nullptr;
    QNetworkReply *m_gatewayProbeReply = nullptr;
    LoginMode m_loginMode = LoginMode::Student;

    QString m_studentUser;
    QString m_studentPassword;
    QString m_studentServer = QStringLiteral("aust");
    QString m_teacherUser;
    QString m_teacherPassword;
    QString m_statusText = QStringLiteral("准备就绪");
    QString m_networkStatusText = QStringLiteral("正在检测网络状态...");
    QString m_backgroundServiceStatusText = QStringLiteral("后台守护服务未开启");
    QString m_updateStatusText = QStringLiteral("正在准备更新检查...");
    QString m_updateVersionText;
    QString m_currentVersionName;
    QString m_updateLatestVersion;
    QString m_updateNotes;
    QString m_updateUrl;
    QString m_updateSha256;
    QString m_pendingApkPath;
    QString m_currentSsid;
    QString m_currentNetworkKey;
    QString m_missingNetworkPermissions;
    QDateTime m_lastAutoLoginAttempt;
    QDateTime m_lastSuccessfulLogin;
    QDateTime m_lastServiceTick;
    qint64 m_updateSize = 0;
    int m_currentVersionCode = 0;
    int m_updateVersionCode = 0;
    qreal m_updateDownloadProgress = 0.0;
    bool m_autoLoginOnLaunch = false;
    bool m_autoLoginOnlyOnCampusWifi = true;
    bool m_backgroundServiceEnabled = false;
    bool m_notificationPermissionGranted = true;
    bool m_installPermissionGranted = true;
    bool m_updateAvailable = false;
    bool m_updateBusy = false;
    bool m_wifiConnected = false;
    bool m_campusGatewayReachable = false;
    bool m_campusWifiDetected = false;
    bool m_busy = false;
};

#endif // MOBILEBACKEND_H
