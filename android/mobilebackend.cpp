#include "mobilebackend.h"

#include "credentialstore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#include <atomic>
#include <jni.h>
#endif

namespace {
constexpr char kStudentLoginUrl[] = "http://10.255.0.19/a79.htm";
constexpr char kTeacherLoginUrl[] = "http://10.255.0.19/drcom/login";
constexpr char kCampusGatewayProbeUrl[] = "http://10.255.0.19/";
constexpr int kNetworkRefreshIntervalMs = 15000;
constexpr int kAutoLoginCooldownSeconds = 45;
constexpr int kAutoLoginStabilizeDelayMs = 2500;
constexpr int kAutoLoginRetryDelayMs = 3500;
constexpr int kAutoLoginTransientMaxRetries = 2;
constexpr int kCampusGatewayProbeTimeoutMs = 2500;
constexpr char kAndroidUpdateManifestUrl[] = "https://www.meng-xun.top/aust-wifi/android/update.json";
constexpr char kAndroidPackageName[] = "top.mengxun.austwifi";

#ifdef Q_OS_ANDROID
constexpr char kAndroidNetworkHelperClass[] = "top/mengxun/austwifi/NetworkStateHelper";
constexpr char kAndroidForegroundServiceClass[] = "top/mengxun/austwifi/AustWifiForegroundService";
constexpr char kAndroidApkUpdateHelperClass[] = "top/mengxun/austwifi/ApkUpdateHelper";
std::atomic<MobileBackend *> s_mobileBackend = nullptr;

QString callAndroidNetworkState()
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const QJniObject result = QJniObject::callStaticObjectMethod(
        kAndroidNetworkHelperClass,
        "networkState",
        "(Landroid/content/Context;)Ljava/lang/String;",
        context.object<jobject>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent) || !result.isValid()) {
        return {};
    }
    return result.toString();
}

bool requestAndroidNetworkPermissions()
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const jboolean requested = QJniObject::callStaticMethod<jboolean>(
        kAndroidNetworkHelperClass,
        "requestNetworkPermissions",
        "(Landroid/content/Context;)Z",
        context.object<jobject>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent)) {
        return false;
    }
    return requested;
}

bool callAndroidNetworkBoolMethod(const char *methodName)
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const jboolean ok = QJniObject::callStaticMethod<jboolean>(
        kAndroidNetworkHelperClass,
        methodName,
        "(Landroid/content/Context;)Z",
        context.object<jobject>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent)) {
        return false;
    }
    return ok;
}

bool callAndroidForegroundServiceMethod(const char *methodName)
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const jboolean ok = QJniObject::callStaticMethod<jboolean>(
        kAndroidForegroundServiceClass,
        methodName,
        "(Landroid/content/Context;)Z",
        context.object<jobject>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent)) {
        return false;
    }
    return ok;
}

QString callAndroidApkUpdateStringMethod(const char *methodName)
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const QJniObject result = QJniObject::callStaticObjectMethod(
        kAndroidApkUpdateHelperClass,
        methodName,
        "(Landroid/content/Context;)Ljava/lang/String;",
        context.object<jobject>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent) || !result.isValid()) {
        return {};
    }
    return result.toString();
}

bool callAndroidApkUpdateBoolMethod(const char *methodName)
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const jboolean ok = QJniObject::callStaticMethod<jboolean>(
        kAndroidApkUpdateHelperClass,
        methodName,
        "(Landroid/content/Context;)Z",
        context.object<jobject>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent)) {
        return false;
    }
    return ok;
}

QString callAndroidApkUpdateStringPathMethod(const char *methodName, const QString &path)
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const QJniObject pathArg = QJniObject::fromString(path);
    const QJniObject result = QJniObject::callStaticObjectMethod(
        kAndroidApkUpdateHelperClass,
        methodName,
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        context.object<jobject>(),
        pathArg.object<jstring>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent) || !result.isValid()) {
        return {};
    }
    return result.toString();
}

bool callAndroidInstallApk(const QString &apkPath)
{
    const auto context = QNativeInterface::QAndroidApplication::context();
    const QJniObject pathArg = QJniObject::fromString(apkPath);
    const jboolean ok = QJniObject::callStaticMethod<jboolean>(
        kAndroidApkUpdateHelperClass,
        "installApk",
        "(Landroid/content/Context;Ljava/lang/String;)Z",
        context.object<jobject>(),
        pathArg.object<jstring>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent)) {
        return false;
    }
    return ok;
}
#endif
}

#ifdef Q_OS_ANDROID
extern "C" JNIEXPORT void JNICALL
Java_top_mengxun_austwifi_AustWifiForegroundService_nativeGuardTick(JNIEnv *, jclass)
{
    MobileBackend *backend = s_mobileBackend.load(std::memory_order_acquire);
    if (!backend) {
        return;
    }

    QMetaObject::invokeMethod(backend, "handleBackgroundServiceTick", Qt::QueuedConnection);
}
#endif

MobileBackend::MobileBackend(QObject *parent)
    : QObject(parent)
    , m_settings("AUST_WIFI", "Android")
{
#ifdef Q_OS_ANDROID
    s_mobileBackend.store(this, std::memory_order_release);
#endif

    loadConfig();
    refreshCurrentAppVersion();
    m_autoLoginTimer.setSingleShot(true);
    connect(&m_autoLoginTimer, &QTimer::timeout, this, &MobileBackend::performScheduledAutoLogin);
    connect(&m_networkRefreshTimer, &QTimer::timeout, this, &MobileBackend::refreshNetworkState);
    updateNetworkRefreshTimer();
    QTimer::singleShot(400, this, &MobileBackend::refreshNetworkState);
    QTimer::singleShot(600, this, &MobileBackend::refreshNotificationPermission);
    QTimer::singleShot(700, this, &MobileBackend::refreshInstallPermission);
    QTimer::singleShot(900, this, &MobileBackend::syncBackgroundServiceState);
    QTimer::singleShot(1200, this, &MobileBackend::runStartupAutoLogin);
    QTimer::singleShot(1800, this, &MobileBackend::checkForUpdates);
}

MobileBackend::~MobileBackend()
{
    cancelLogin();
    clearGatewayProbe();
    clearUpdateReplies();
#ifdef Q_OS_ANDROID
    if (s_mobileBackend.load(std::memory_order_acquire) == this) {
        s_mobileBackend.store(nullptr, std::memory_order_release);
    }
#endif
}

QString MobileBackend::studentUser() const
{
    return m_studentUser;
}

QString MobileBackend::studentPassword() const
{
    return m_studentPassword;
}

QString MobileBackend::studentServer() const
{
    return m_studentServer;
}

QString MobileBackend::teacherUser() const
{
    return m_teacherUser;
}

QString MobileBackend::teacherPassword() const
{
    return m_teacherPassword;
}

QString MobileBackend::statusText() const
{
    return m_statusText;
}

QString MobileBackend::activeAccountText() const
{
    if (!m_teacherUser.trimmed().isEmpty() && !m_teacherPassword.isEmpty()) {
        return QStringLiteral("当前将优先使用教师账号登录");
    }
    if (!m_studentUser.trimmed().isEmpty() && !m_studentPassword.isEmpty()) {
        return QStringLiteral("当前将使用学生账号登录");
    }
    return QStringLiteral("请先填写至少一套账号");
}

QString MobileBackend::credentialBackendText() const
{
    return QStringLiteral("密码存储：%1").arg(CredentialStore::backendName());
}

QString MobileBackend::networkStatusText() const
{
    return m_networkStatusText;
}

QString MobileBackend::currentSsid() const
{
    return m_currentSsid;
}

bool MobileBackend::autoLoginOnLaunch() const
{
    return m_autoLoginOnLaunch;
}

bool MobileBackend::autoLoginOnlyOnCampusWifi() const
{
    return m_autoLoginOnlyOnCampusWifi;
}

bool MobileBackend::wifiConnected() const
{
    return m_wifiConnected;
}

bool MobileBackend::campusWifiDetected() const
{
    return m_campusWifiDetected;
}

bool MobileBackend::campusGatewayReachable() const
{
    return m_campusGatewayReachable;
}

bool MobileBackend::backgroundServiceEnabled() const
{
    return m_backgroundServiceEnabled;
}

bool MobileBackend::notificationPermissionGranted() const
{
    return m_notificationPermissionGranted;
}

QString MobileBackend::backgroundServiceStatusText() const
{
    return m_backgroundServiceStatusText;
}

QString MobileBackend::updateStatusText() const
{
    return m_updateStatusText;
}

QString MobileBackend::updateVersionText() const
{
    return m_updateVersionText;
}

bool MobileBackend::updateAvailable() const
{
    return m_updateAvailable;
}

bool MobileBackend::updateBusy() const
{
    return m_updateBusy;
}

bool MobileBackend::installPermissionGranted() const
{
    return m_installPermissionGranted;
}

qreal MobileBackend::updateDownloadProgress() const
{
    return m_updateDownloadProgress;
}

bool MobileBackend::busy() const
{
    return m_busy;
}

void MobileBackend::setStudentUser(const QString &value)
{
    if (m_studentUser == value) {
        return;
    }
    m_studentUser = value;
    emit configChanged();
}

void MobileBackend::setStudentPassword(const QString &value)
{
    if (m_studentPassword == value) {
        return;
    }
    m_studentPassword = value;
    emit configChanged();
}

void MobileBackend::setStudentServer(const QString &value)
{
    const QString normalized = value.trimmed().isEmpty() ? QStringLiteral("aust") : value.trimmed();
    if (m_studentServer == normalized) {
        return;
    }
    m_studentServer = normalized;
    emit configChanged();
}

void MobileBackend::setTeacherUser(const QString &value)
{
    if (m_teacherUser == value) {
        return;
    }
    m_teacherUser = value;
    emit configChanged();
}

void MobileBackend::setTeacherPassword(const QString &value)
{
    if (m_teacherPassword == value) {
        return;
    }
    m_teacherPassword = value;
    emit configChanged();
}

void MobileBackend::setAutoLoginOnLaunch(bool value)
{
    if (m_autoLoginOnLaunch == value) {
        return;
    }
    m_autoLoginOnLaunch = value;
    m_settings.setValue("autoLogin/onLaunch", m_autoLoginOnLaunch);
    m_settings.sync();
    emit configChanged();
}

void MobileBackend::setAutoLoginOnlyOnCampusWifi(bool value)
{
    if (m_autoLoginOnlyOnCampusWifi == value) {
        return;
    }
    m_autoLoginOnlyOnCampusWifi = value;
    m_settings.setValue("autoLogin/onlyCampusWifi", m_autoLoginOnlyOnCampusWifi);
    m_settings.sync();
    emit configChanged();
    evaluateAutoLoginSchedule();
}

void MobileBackend::setBackgroundServiceEnabled(bool value)
{
    if (m_backgroundServiceEnabled == value) {
        return;
    }
    m_backgroundServiceEnabled = value;
    m_settings.setValue("backgroundService/enabled", m_backgroundServiceEnabled);
    m_settings.sync();
    emit serviceStateChanged();
    updateNetworkRefreshTimer();
    syncBackgroundServiceState();
}

void MobileBackend::loadConfig()
{
    CredentialStore credentials(&m_settings);
    credentials.migrateLegacyPassword("student");
    credentials.migrateLegacyPassword("teacher");

    m_studentUser = m_settings.value("student/user").toString();
    m_studentPassword = credentials.password("student");
    m_studentServer = m_settings.value("student/server", "aust").toString();
    m_teacherUser = m_settings.value("teacher/user").toString();
    m_teacherPassword = credentials.password("teacher");
    m_autoLoginOnLaunch = m_settings.value("autoLogin/onLaunch", false).toBool();
    m_autoLoginOnlyOnCampusWifi = m_settings.value("autoLogin/onlyCampusWifi", true).toBool();
    m_backgroundServiceEnabled = m_settings.value("backgroundService/enabled", false).toBool();

    emit configChanged();
    emit serviceStateChanged();
}

bool MobileBackend::saveConfig()
{
    CredentialStore credentials(&m_settings);

    m_settings.setValue("student/user", m_studentUser.trimmed());
    m_settings.setValue("student/server", m_studentServer.trimmed().isEmpty() ? QStringLiteral("aust") : m_studentServer.trimmed());
    if (!credentials.setPassword("student", m_studentPassword)) {
        setStatusText(QStringLiteral("学生密码保存失败"));
        return false;
    }

    m_settings.setValue("teacher/user", m_teacherUser.trimmed());
    m_settings.setValue("teacher/server", "jzg");
    m_settings.setValue("autoLogin/onLaunch", m_autoLoginOnLaunch);
    m_settings.setValue("autoLogin/onlyCampusWifi", m_autoLoginOnlyOnCampusWifi);
    m_settings.setValue("backgroundService/enabled", m_backgroundServiceEnabled);
    if (!credentials.setPassword("teacher", m_teacherPassword)) {
        setStatusText(QStringLiteral("教师密码保存失败"));
        return false;
    }

    m_settings.sync();
    setStatusText(QStringLiteral("配置已保存"));
    return true;
}

void MobileBackend::login()
{
    startLogin(false);
}

void MobileBackend::startLogin(bool automatic)
{
    if (m_busy) {
        setStatusText(QStringLiteral("登录请求正在进行，请稍后"));
        return;
    }

    m_currentLoginAutomatic = automatic;
    if (!automatic) {
        m_autoLoginTimer.stop();
        m_autoLoginTransientRetryCount = 0;
    }

    if (!saveConfig()) {
        m_currentLoginAutomatic = false;
        emit loginFailed(m_statusText);
        return;
    }

    if (!m_wifiConnected) {
        m_currentLoginAutomatic = false;
        setStatusText(QStringLiteral("请先连接 WiFi 后再登录校园网"));
        emit loginFailed(m_statusText);
        return;
    }

    const QString teacherUser = m_teacherUser.trimmed();
    if (!teacherUser.isEmpty() && !m_teacherPassword.isEmpty()) {
        startTeacherLogin(teacherUser, m_teacherPassword);
        return;
    }

    const QString studentUser = m_studentUser.trimmed();
    if (!studentUser.isEmpty() && !m_studentPassword.isEmpty()) {
        startStudentLogin(studentUser, m_studentPassword, m_studentServer.trimmed().isEmpty() ? QStringLiteral("aust") : m_studentServer.trimmed());
        return;
    }

    setStatusText(QStringLiteral("请先填写学生或教师账号密码"));
    m_currentLoginAutomatic = false;
    emit loginFailed(m_statusText);
}

void MobileBackend::cancelLogin()
{
    m_autoLoginTimer.stop();
    m_currentLoginAutomatic = false;
    m_autoLoginTransientRetryCount = 0;
    if (m_reply) {
        m_reply->abort();
    }
    clearReply();
    setBusy(false);
}

void MobileBackend::refreshNetworkState()
{
#ifdef Q_OS_ANDROID
    const QString stateJson = callAndroidNetworkState();
    if (stateJson.isEmpty()) {
        m_networkStatusText = QStringLiteral("无法读取 Android 网络状态");
        emit networkStateChanged();
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(stateJson.toUtf8());
    const QJsonObject object = document.object();
    const bool wifiConnected = object.value("wifiConnected").toBool(false);
    const QString ssid = object.value("ssid").toString();
    const QString networkKey = object.value("networkKey").toString();
    const QString missingPermissions = object.value("missingPermissions").toString();

    const bool oldWifiConnected = m_wifiConnected;
    const bool oldCampusGatewayReachable = m_campusGatewayReachable;
    const bool oldCampusWifiDetected = m_campusWifiDetected;
    const QString oldSsid = m_currentSsid;
    const QString oldNetworkKey = m_currentNetworkKey;
    const QString oldMissingPermissions = m_missingNetworkPermissions;
    const QString oldStatus = m_networkStatusText;
    const bool networkChanged = oldNetworkKey != networkKey;
    const bool shouldCancelLogin = m_reply && (oldWifiConnected != wifiConnected || networkChanged);

    if (!wifiConnected || networkChanged) {
        m_autoLoginTimer.stop();
        m_autoLoginTransientRetryCount = 0;
    }

    m_wifiConnected = wifiConnected;
    m_currentSsid = ssid;
    m_currentNetworkKey = networkKey;
    m_missingNetworkPermissions = missingPermissions;
    if (!m_wifiConnected || networkChanged) {
        m_campusGatewayReachable = false;
    }
    m_campusWifiDetected = m_wifiConnected && (isCampusWifiSsid(m_currentSsid) || m_campusGatewayReachable);
    m_networkStatusText = buildNetworkStatusText();

    if (m_wifiConnected) {
        if (networkChanged) {
            clearGatewayProbe();
        }
        startGatewayProbe();
    } else {
        clearGatewayProbe();
    }

    const bool changed = oldWifiConnected != m_wifiConnected
        || oldCampusGatewayReachable != m_campusGatewayReachable
        || oldCampusWifiDetected != m_campusWifiDetected
        || oldSsid != m_currentSsid
        || oldNetworkKey != m_currentNetworkKey
        || oldMissingPermissions != m_missingNetworkPermissions
        || oldStatus != m_networkStatusText;

    if (changed) {
        emit networkStateChanged();
    }

    if (shouldCancelLogin) {
        cancelLogin();
        setStatusText(m_wifiConnected
            ? QStringLiteral("网络环境已变化，已取消本次登录")
            : QStringLiteral("WiFi 已断开，已取消本次登录"));
    }
    evaluateAutoLoginSchedule();
#else
    m_networkStatusText = QStringLiteral("网络状态识别仅 Android 端可用");
    emit networkStateChanged();
#endif
}

void MobileBackend::requestNetworkPermissions()
{
#ifdef Q_OS_ANDROID
    const bool requested = requestAndroidNetworkPermissions();
    if (requested) {
        setStatusText(QStringLiteral("已请求 WiFi 识别权限，请在系统弹窗中允许"));
    } else if (m_missingNetworkPermissions.isEmpty()) {
        setStatusText(QStringLiteral("WiFi 识别权限已具备"));
    } else {
        setStatusText(QStringLiteral("无法弹出权限请求，请到系统设置中允许：%1").arg(m_missingNetworkPermissions));
    }
    QTimer::singleShot(2500, this, &MobileBackend::refreshNetworkState);
#else
    setStatusText(QStringLiteral("网络权限请求仅 Android 端可用"));
#endif
}

void MobileBackend::requestNotificationPermission()
{
#ifdef Q_OS_ANDROID
    const bool requested = callAndroidForegroundServiceMethod("requestNotificationPermission");
    refreshNotificationPermission();
    if (requested) {
        setStatusText(QStringLiteral("已请求通知权限，请在系统弹窗中允许"));
    } else if (m_notificationPermissionGranted) {
        setStatusText(QStringLiteral("通知权限已具备"));
    } else {
        setStatusText(QStringLiteral("无法弹出通知权限请求，请到系统设置中允许通知"));
    }
    QTimer::singleShot(2500, this, &MobileBackend::refreshNotificationPermission);
#else
    setStatusText(QStringLiteral("通知权限请求仅 Android 端可用"));
#endif
}

void MobileBackend::checkForUpdates()
{
    if (m_updateBusy) {
        return;
    }
    if (m_busy || m_wifiNetworkBindingUsers > 0 || m_autoLoginTimer.isActive()) {
        if (!m_updateCheckDeferred) {
            m_updateCheckDeferred = true;
            setUpdateStatusText(QStringLiteral("登录完成后自动检查更新"));
        }
        QTimer::singleShot(5000, this, &MobileBackend::checkForUpdates);
        return;
    }
    m_updateCheckDeferred = false;

    if (m_updateManifestReply) {
        disconnect(m_updateManifestReply, nullptr, this, nullptr);
        m_updateManifestReply->deleteLater();
        m_updateManifestReply = nullptr;
    }

    refreshCurrentAppVersion();
    refreshInstallPermission();
    m_updateAvailable = false;
    m_updateLatestVersion.clear();
    m_updateVersionCode = 0;
    m_updateNotes.clear();
    m_updateUrl.clear();
    m_updateSha256.clear();
    m_updateSize = 0;
    m_updateDownloadProgress = 0.0;
    setUpdateBusy(true);
    setUpdateStatusText(QStringLiteral("正在检查 Android 更新..."));
    emit updateStateChanged();

    QNetworkRequest request(QUrl(QString::fromLatin1(kAndroidUpdateManifestUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "AUST-WiFi-Android-Updater");
    request.setRawHeader("Cache-Control", "no-cache");
    request.setTransferTimeout(15000);

    m_updateManifestReply = m_updateNetworkManager.get(request);
    connect(m_updateManifestReply, &QNetworkReply::finished, this, &MobileBackend::finishUpdateCheck);
}

void MobileBackend::downloadAndInstallUpdate()
{
    if (m_updateBusy) {
        return;
    }

    refreshInstallPermission();
    if (!m_installPermissionGranted) {
        setUpdateStatusText(QStringLiteral("请先允许本应用安装未知来源 APK"));
        openInstallPermissionSettings();
        return;
    }

    if (!m_pendingApkPath.isEmpty() && QFile::exists(m_pendingApkPath)) {
        installDownloadedUpdate();
        return;
    }

    if (!m_updateAvailable || m_updateUrl.isEmpty()) {
        setUpdateStatusText(QStringLiteral("暂无可下载的新版本，请先检查更新"));
        return;
    }

    const QUrl url(m_updateUrl);
    if (!url.isValid() || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) != 0) {
        setUpdateStatusText(QStringLiteral("更新下载地址无效，已拒绝下载"));
        return;
    }
    if (m_updateSha256.size() != 64) {
        setUpdateStatusText(QStringLiteral("更新清单缺少有效 SHA-256，已拒绝下载"));
        return;
    }

    if (m_updateDownloadReply) {
        disconnect(m_updateDownloadReply, nullptr, this, nullptr);
        m_updateDownloadReply->deleteLater();
        m_updateDownloadReply = nullptr;
    }

    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString updateDir = QDir(cacheRoot).filePath(QStringLiteral("updates"));
    if (!QDir().mkpath(updateDir)) {
        setUpdateStatusText(QStringLiteral("无法创建 APK 下载目录"));
        return;
    }

    const QString safeVersion = m_updateLatestVersion.isEmpty() ? QStringLiteral("latest") : m_updateLatestVersion;
    m_pendingApkPath = QDir(updateDir).filePath(QStringLiteral("AUST-WIFI-Android-%1.apk").arg(safeVersion));
    QFile::remove(m_pendingApkPath);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "AUST-WiFi-Android-Updater");
    request.setTransferTimeout(120000);

    m_updateDownloadProgress = 0.0;
    setUpdateBusy(true);
    setUpdateStatusText(QStringLiteral("正在下载 Android 更新..."));
    emit updateStateChanged();

    m_updateDownloadReply = m_updateNetworkManager.get(request);
    connect(m_updateDownloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_updateDownloadProgress = qBound<qreal>(0.0, static_cast<qreal>(received) / static_cast<qreal>(total), 1.0);
            emit updateStateChanged();
        }
    });
    connect(m_updateDownloadReply, &QNetworkReply::finished, this, &MobileBackend::finishUpdateDownload);
}

void MobileBackend::openInstallPermissionSettings()
{
#ifdef Q_OS_ANDROID
    const bool opened = callAndroidApkUpdateBoolMethod("openInstallPermissionSettings");
    setUpdateStatusText(opened
        ? QStringLiteral("请在系统设置中允许 AUST WiFi 安装未知应用")
        : QStringLiteral("无法打开安装权限设置，请在系统设置中手动允许"));
    QTimer::singleShot(3000, this, &MobileBackend::refreshInstallPermission);
#else
    setUpdateStatusText(QStringLiteral("安装权限设置仅 Android 端可用"));
#endif
}

QUrlQuery MobileBackend::buildLoginQuery(const QString &user, const QString &server, const QString &password)
{
    QUrlQuery query;
    query.addQueryItem("callback", "dr1003");
    query.addQueryItem("DDDDD", QString("%1@%2").arg(user, server));
    query.addQueryItem("upass", password);
    query.addQueryItem("0MKKey", "123456");
    query.addQueryItem("R1", "0");
    query.addQueryItem("R3", "0");
    query.addQueryItem("R6", "0");
    query.addQueryItem("para", "00");
    query.addQueryItem("v6ip", "");
    query.addQueryItem("v", "");
    return query;
}

void MobileBackend::setStatusText(const QString &value)
{
    if (m_statusText == value) {
        return;
    }
    m_statusText = value;
    emit statusChanged();
}

void MobileBackend::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void MobileBackend::startStudentLogin(const QString &user, const QString &password, const QString &server)
{
    clearReply();
    m_loginUsesWifiNetworkBinding = acquireWifiNetworkBinding();
    if (!m_loginUsesWifiNetworkBinding) {
        m_currentLoginAutomatic = false;
        setStatusText(QStringLiteral("无法将登录请求切换到 WiFi 网络，请关闭移动数据后重试"));
        emit loginFailed(m_statusText);
        return;
    }

    m_loginMode = LoginMode::Student;
    setBusy(true);
    setStatusText(QStringLiteral("正在使用学生账号登录..."));

    QNetworkRequest request{QUrl(kStudentLoginUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setTransferTimeout(12000);

    const QByteArray data = buildLoginQuery(user, server, password).query(QUrl::FullyEncoded).toUtf8();
    m_reply = m_networkManager.post(request, data);
    connect(m_reply, &QNetworkReply::finished, this, &MobileBackend::finishLogin);
}

void MobileBackend::startTeacherLogin(const QString &user, const QString &password)
{
    clearReply();
    m_loginUsesWifiNetworkBinding = acquireWifiNetworkBinding();
    if (!m_loginUsesWifiNetworkBinding) {
        m_currentLoginAutomatic = false;
        setStatusText(QStringLiteral("无法将登录请求切换到 WiFi 网络，请关闭移动数据后重试"));
        emit loginFailed(m_statusText);
        return;
    }

    m_loginMode = LoginMode::Teacher;
    setBusy(true);
    setStatusText(QStringLiteral("正在使用教师账号登录..."));

    QUrl url(kTeacherLoginUrl);
    url.setQuery(buildLoginQuery(user, "jzg", password));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
    request.setTransferTimeout(12000);

    m_reply = m_networkManager.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &MobileBackend::finishLogin);
}

bool MobileBackend::hasUsableCredentials() const
{
    if (!m_teacherUser.trimmed().isEmpty() && !m_teacherPassword.isEmpty()) {
        return true;
    }
    return !m_studentUser.trimmed().isEmpty() && !m_studentPassword.isEmpty();
}

void MobileBackend::runStartupAutoLogin()
{
    if (!m_autoLoginOnLaunch || m_busy) {
        return;
    }
    if (!hasUsableCredentials()) {
        setStatusText(QStringLiteral("启动自动登录已开启，请先填写并保存账号"));
        return;
    }
    if (m_autoLoginOnlyOnCampusWifi && !m_campusWifiDetected) {
        setStatusText(QStringLiteral("启动自动登录已开启，等待连接校园网环境"));
        return;
    }
    if (!m_wifiConnected) {
        setStatusText(QStringLiteral("启动自动登录已开启，等待连接 WiFi"));
        return;
    }

    m_autoLoginTransientRetryCount = 0;
    m_lastAutoLoginAttempt = QDateTime::currentDateTime();
    scheduleAutoLoginAttempt(QStringLiteral("启动自动登录准备中，等待网络稳定..."));
}

void MobileBackend::evaluateAutoLoginSchedule()
{
    if (!m_autoLoginOnLaunch || m_busy || !hasUsableCredentials()) {
        return;
    }
    if (!m_wifiConnected) {
        return;
    }
    if (m_autoLoginOnlyOnCampusWifi && !m_campusWifiDetected) {
        return;
    }
    if (m_autoLoginTimer.isActive()) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    if (m_lastAutoLoginAttempt.isValid() &&
        m_lastAutoLoginAttempt.secsTo(now) < kAutoLoginCooldownSeconds) {
        return;
    }
    if (m_lastSuccessfulLogin.isValid() &&
        m_lastSuccessfulLogin.secsTo(now) < kAutoLoginCooldownSeconds) {
        return;
    }

    m_lastAutoLoginAttempt = now;
    m_autoLoginTransientRetryCount = 0;
    scheduleAutoLoginAttempt(m_campusWifiDetected
        ? QStringLiteral("检测到校园网环境，等待网络稳定后自动登录...")
        : QStringLiteral("检测到可用 WiFi，等待网络稳定后自动登录..."));
}

void MobileBackend::scheduleAutoLoginAttempt(const QString &statusText)
{
    if (m_autoLoginTimer.isActive()) {
        return;
    }
    setStatusText(statusText);
    m_autoLoginTimer.start(kAutoLoginStabilizeDelayMs);
}

void MobileBackend::performScheduledAutoLogin()
{
    if (!m_autoLoginOnLaunch || m_busy || !hasUsableCredentials()) {
        return;
    }
    if (!m_wifiConnected) {
        setStatusText(QStringLiteral("自动登录已等待，当前未连接 WiFi"));
        return;
    }
    if (m_autoLoginOnlyOnCampusWifi && !m_campusWifiDetected) {
        setStatusText(QStringLiteral("自动登录已等待，当前不是校园网环境"));
        return;
    }

    setStatusText(m_campusWifiDetected
        ? QStringLiteral("检测到校园网环境，正在自动登录...")
        : QStringLiteral("检测到可用 WiFi，正在自动登录..."));
    startLogin(true);
}

bool MobileBackend::scheduleAutoLoginRetry(int statusCode)
{
    if (!m_currentLoginAutomatic || m_autoLoginTransientRetryCount >= kAutoLoginTransientMaxRetries) {
        return false;
    }
    if (!m_autoLoginOnLaunch || !m_wifiConnected || (m_autoLoginOnlyOnCampusWifi && !m_campusWifiDetected)) {
        return false;
    }

    ++m_autoLoginTransientRetryCount;
    setStatusText(QStringLiteral("自动登录响应未确认成功（HTTP %1），稍后重试 %2/%3")
        .arg(statusCode)
        .arg(m_autoLoginTransientRetryCount)
        .arg(kAutoLoginTransientMaxRetries));
    m_autoLoginTimer.start(kAutoLoginRetryDelayMs);
    return true;
}

void MobileBackend::finishUpdateCheck()
{
    QNetworkReply *reply = m_updateManifestReply;
    m_updateManifestReply = nullptr;
    setUpdateBusy(false);

    if (!reply) {
        setUpdateStatusText(QStringLiteral("更新检查状态异常"));
        return;
    }

    const QNetworkReply::NetworkError error = reply->error();
    const QString errorText = reply->errorString();
    const QByteArray payload = reply->readAll();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        setUpdateStatusText(QStringLiteral("更新检查失败：%1").arg(errorText));
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setUpdateStatusText(QStringLiteral("更新清单格式无效"));
        return;
    }

    const QJsonObject object = document.object();
    const QString platform = object.value(QStringLiteral("platform")).toString();
    const QString packageName = object.value(QStringLiteral("package")).toString();
    const int versionCode = object.value(QStringLiteral("version_code")).toInt();
    const QString latest = object.value(QStringLiteral("latest")).toString();
    const QString url = object.value(QStringLiteral("url")).toString();
    const QString sha256 = object.value(QStringLiteral("sha256")).toString().trimmed().toLower();
    const qint64 size = static_cast<qint64>(object.value(QStringLiteral("size")).toDouble());
    const QString notes = object.value(QStringLiteral("notes")).toString();
    const bool signedApk = object.value(QStringLiteral("signed")).toBool(false);

    if (platform != QStringLiteral("android") || packageName != QString::fromLatin1(kAndroidPackageName)) {
        setUpdateStatusText(QStringLiteral("更新清单不属于当前 Android 应用"));
        return;
    }
    if (versionCode <= 0 || latest.isEmpty()) {
        setUpdateStatusText(QStringLiteral("更新清单缺少版本信息"));
        return;
    }
    if (!signedApk) {
        setUpdateStatusText(QStringLiteral("更新清单未标记为正式签名 APK，已拒绝"));
        return;
    }

    m_updateLatestVersion = latest;
    m_updateVersionCode = versionCode;
    m_updateNotes = notes;
    m_updateUrl = url;
    m_updateSha256 = sha256;
    m_updateSize = size;
    m_pendingApkPath.clear();

    if (versionCode <= m_currentVersionCode) {
        m_updateAvailable = false;
        setUpdateStatusText(QStringLiteral("当前已是最新 Android 版本"));
        emit updateStateChanged();
        return;
    }

    m_updateAvailable = true;
    setUpdateStatusText(QStringLiteral("发现新版本 %1（%2），可下载更新").arg(latest).arg(versionCode));
    emit updateStateChanged();
}

void MobileBackend::finishUpdateDownload()
{
    QNetworkReply *reply = m_updateDownloadReply;
    m_updateDownloadReply = nullptr;
    setUpdateBusy(false);

    if (!reply) {
        setUpdateStatusText(QStringLiteral("更新下载状态异常"));
        return;
    }

    const QNetworkReply::NetworkError error = reply->error();
    const QString errorText = reply->errorString();
    const QByteArray data = reply->readAll();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        setUpdateStatusText(QStringLiteral("更新下载失败：%1").arg(errorText));
        QFile::remove(m_pendingApkPath);
        return;
    }

    const QString actualHash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    if (!m_updateSha256.isEmpty() && actualHash.compare(m_updateSha256, Qt::CaseInsensitive) != 0) {
        setUpdateStatusText(QStringLiteral("APK 校验失败，已取消安装"));
        QFile::remove(m_pendingApkPath);
        m_pendingApkPath.clear();
        return;
    }
    if (m_updateSize > 0 && data.size() != m_updateSize) {
        setUpdateStatusText(QStringLiteral("APK 大小不一致，已取消安装"));
        QFile::remove(m_pendingApkPath);
        m_pendingApkPath.clear();
        return;
    }

    QFile file(m_pendingApkPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setUpdateStatusText(QStringLiteral("无法保存下载的 APK"));
        m_pendingApkPath.clear();
        return;
    }
    if (file.write(data) != data.size()) {
        file.close();
        QFile::remove(m_pendingApkPath);
        m_pendingApkPath.clear();
        setUpdateStatusText(QStringLiteral("APK 写入不完整，已取消安装"));
        return;
    }
    file.close();

#ifdef Q_OS_ANDROID
    const QString apkInfoJson = callAndroidApkUpdateStringPathMethod("apkInfo", m_pendingApkPath);
    const QJsonDocument apkInfoDocument = QJsonDocument::fromJson(apkInfoJson.toUtf8());
    const QJsonObject apkInfo = apkInfoDocument.object();
    const QString apkPackageName = apkInfo.value(QStringLiteral("packageName")).toString();
    const int apkVersionCode = apkInfo.value(QStringLiteral("versionCode")).toInt();
    if (apkPackageName != QString::fromLatin1(kAndroidPackageName) || apkVersionCode != m_updateVersionCode) {
        QFile::remove(m_pendingApkPath);
        m_pendingApkPath.clear();
        setUpdateStatusText(QStringLiteral("APK 包名或版本号不匹配，已取消安装"));
        return;
    }
#endif

    m_updateDownloadProgress = 1.0;
    emit updateStateChanged();
    installDownloadedUpdate();
}

void MobileBackend::handleBackgroundServiceTick()
{
    if (!m_backgroundServiceEnabled) {
        return;
    }

    m_lastServiceTick = QDateTime::currentDateTime();
    setBackgroundServiceStatusText(
        QStringLiteral("后台守护服务运行中，%1 已检查 WiFi")
            .arg(m_lastServiceTick.time().toString(QStringLiteral("HH:mm:ss"))));
    refreshNetworkState();
}

void MobileBackend::syncBackgroundServiceState()
{
#ifdef Q_OS_ANDROID
    refreshNotificationPermission();
    if (m_backgroundServiceEnabled) {
        const bool started = callAndroidForegroundServiceMethod("start");
        setBackgroundServiceStatusText(started
            ? QStringLiteral("后台守护服务运行中，正在定时检查 WiFi")
            : QStringLiteral("后台守护服务启动失败，请检查通知权限"));
        if (!started) {
            m_backgroundServiceEnabled = false;
            m_settings.setValue("backgroundService/enabled", false);
            m_settings.sync();
            updateNetworkRefreshTimer();
            emit serviceStateChanged();
        }
        return;
    }

    callAndroidForegroundServiceMethod("stop");
    updateNetworkRefreshTimer();
    setBackgroundServiceStatusText(QStringLiteral("后台守护服务未开启"));
#else
    setBackgroundServiceStatusText(QStringLiteral("后台守护服务仅 Android 端可用"));
#endif
}

void MobileBackend::updateNetworkRefreshTimer()
{
    if (m_backgroundServiceEnabled) {
        m_networkRefreshTimer.stop();
        return;
    }

    if (!m_networkRefreshTimer.isActive()) {
        m_networkRefreshTimer.start(kNetworkRefreshIntervalMs);
    }
}

void MobileBackend::refreshNotificationPermission()
{
#ifdef Q_OS_ANDROID
    const bool granted = callAndroidForegroundServiceMethod("hasNotificationPermission");
    if (m_notificationPermissionGranted == granted) {
        return;
    }
    m_notificationPermissionGranted = granted;
    emit serviceStateChanged();
#else
    if (!m_notificationPermissionGranted) {
        m_notificationPermissionGranted = true;
        emit serviceStateChanged();
    }
#endif
}

void MobileBackend::setBackgroundServiceStatusText(const QString &value)
{
    if (m_backgroundServiceStatusText == value) {
        return;
    }
    m_backgroundServiceStatusText = value;
    emit serviceStateChanged();
}

void MobileBackend::refreshInstallPermission()
{
#ifdef Q_OS_ANDROID
    const bool granted = callAndroidApkUpdateBoolMethod("canInstallPackages");
    if (m_installPermissionGranted == granted) {
        return;
    }
    m_installPermissionGranted = granted;
    emit updateStateChanged();
#else
    if (!m_installPermissionGranted) {
        m_installPermissionGranted = true;
        emit updateStateChanged();
    }
#endif
}

void MobileBackend::refreshCurrentAppVersion()
{
#ifdef Q_OS_ANDROID
    const QString versionJson = callAndroidApkUpdateStringMethod("versionInfo");
    const QJsonDocument document = QJsonDocument::fromJson(versionJson.toUtf8());
    const QJsonObject object = document.object();
    const QString versionName = object.value(QStringLiteral("versionName")).toString();
    const int versionCode = object.value(QStringLiteral("versionCode")).toInt();
    if (!versionName.isEmpty()) {
        m_currentVersionName = versionName;
    }
    if (versionCode > 0) {
        m_currentVersionCode = versionCode;
    }
#else
    if (m_currentVersionName.isEmpty()) {
        m_currentVersionName = QStringLiteral("unknown");
    }
#endif

    const QString name = m_currentVersionName.isEmpty() ? QStringLiteral("unknown") : m_currentVersionName;
    const QString versionText = QStringLiteral("当前版本：%1（%2）").arg(name).arg(m_currentVersionCode);
    if (m_updateVersionText != versionText) {
        m_updateVersionText = versionText;
        emit updateStateChanged();
    }
}

void MobileBackend::setUpdateStatusText(const QString &value)
{
    if (m_updateStatusText == value) {
        return;
    }
    m_updateStatusText = value;
    emit updateStateChanged();
}

void MobileBackend::setUpdateBusy(bool value)
{
    if (m_updateBusy == value) {
        return;
    }
    m_updateBusy = value;
    emit updateStateChanged();
}

bool MobileBackend::installDownloadedUpdate()
{
    if (m_pendingApkPath.isEmpty() || !QFile::exists(m_pendingApkPath)) {
        setUpdateStatusText(QStringLiteral("未找到已下载的 APK"));
        return false;
    }

    refreshInstallPermission();
    if (!m_installPermissionGranted) {
        setUpdateStatusText(QStringLiteral("APK 已下载，请允许安装未知应用后再次安装"));
        openInstallPermissionSettings();
        return false;
    }

#ifdef Q_OS_ANDROID
    const bool started = callAndroidInstallApk(m_pendingApkPath);
    setUpdateStatusText(started
        ? QStringLiteral("已打开系统安装器，请按提示完成安装")
        : QStringLiteral("无法打开系统安装器"));
    return started;
#else
    setUpdateStatusText(QStringLiteral("APK 安装仅 Android 端可用"));
    return false;
#endif
}

void MobileBackend::clearUpdateReplies()
{
    if (m_updateManifestReply) {
        disconnect(m_updateManifestReply, nullptr, this, nullptr);
        m_updateManifestReply->deleteLater();
        m_updateManifestReply = nullptr;
    }
    if (m_updateDownloadReply) {
        disconnect(m_updateDownloadReply, nullptr, this, nullptr);
        m_updateDownloadReply->deleteLater();
        m_updateDownloadReply = nullptr;
    }
    setUpdateBusy(false);
}

QString MobileBackend::buildNetworkStatusText() const
{
    if (!m_wifiConnected) {
        return QStringLiteral("当前未连接 WiFi");
    }

    const bool hasSsid = !m_currentSsid.trimmed().isEmpty();
    const bool campusBySsid = isCampusWifiSsid(m_currentSsid);

    if (!m_missingNetworkPermissions.isEmpty() && !m_campusGatewayReachable) {
        return QStringLiteral("需要授权%1后才能识别当前 WiFi").arg(m_missingNetworkPermissions);
    }
    if (!m_missingNetworkPermissions.isEmpty() && m_campusGatewayReachable) {
        return QStringLiteral("已连接校园网认证环境（通过认证网关识别）");
    }
    if (!hasSsid) {
        return m_campusGatewayReachable
            ? QStringLiteral("已连接校园网认证环境（通过认证网关识别）")
            : QStringLiteral("已连接 WiFi，但系统未返回 SSID");
    }
    if (campusBySsid) {
        return QStringLiteral("已连接校园 WiFi：%1").arg(m_currentSsid);
    }
    if (m_campusGatewayReachable) {
        return QStringLiteral("已连接校园网认证环境：%1（通过认证网关识别）").arg(m_currentSsid);
    }
    return QStringLiteral("当前 WiFi：%1").arg(m_currentSsid);
}

bool MobileBackend::acquireWifiNetworkBinding()
{
#ifdef Q_OS_ANDROID
    if (!callAndroidNetworkBoolMethod("bindProcessToWifi")) {
        return false;
    }
    ++m_wifiNetworkBindingUsers;
    return true;
#else
    ++m_wifiNetworkBindingUsers;
    return true;
#endif
}

void MobileBackend::releaseWifiNetworkBinding()
{
    if (m_wifiNetworkBindingUsers <= 0) {
        m_wifiNetworkBindingUsers = 0;
        return;
    }

    --m_wifiNetworkBindingUsers;
    if (m_wifiNetworkBindingUsers > 0) {
        return;
    }

#ifdef Q_OS_ANDROID
    callAndroidNetworkBoolMethod("clearProcessNetworkBinding");
#endif
}

void MobileBackend::releaseLoginWifiNetworkBinding()
{
    if (!m_loginUsesWifiNetworkBinding) {
        return;
    }
    m_loginUsesWifiNetworkBinding = false;
    releaseWifiNetworkBinding();
}

void MobileBackend::releaseGatewayProbeWifiNetworkBinding()
{
    if (!m_gatewayProbeUsesWifiNetworkBinding) {
        return;
    }
    m_gatewayProbeUsesWifiNetworkBinding = false;
    releaseWifiNetworkBinding();
}

void MobileBackend::startGatewayProbe()
{
    if (m_gatewayProbeReply || !m_wifiConnected) {
        return;
    }

    m_gatewayProbeUsesWifiNetworkBinding = acquireWifiNetworkBinding();
    if (!m_gatewayProbeUsesWifiNetworkBinding) {
        return;
    }

    QNetworkRequest request(QUrl(QString::fromLatin1(kCampusGatewayProbeUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "AUST-WiFi-Gateway-Probe");
    request.setTransferTimeout(kCampusGatewayProbeTimeoutMs);

    m_gatewayProbeReply = m_gatewayProbeManager.get(request);
    connect(m_gatewayProbeReply, &QNetworkReply::finished, this, &MobileBackend::finishGatewayProbe);
}

void MobileBackend::finishGatewayProbe()
{
    QNetworkReply *reply = m_gatewayProbeReply;
    m_gatewayProbeReply = nullptr;

    if (!reply) {
        return;
    }

    const QNetworkReply::NetworkError error = reply->error();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool reachable = statusCode > 0 || error == QNetworkReply::NoError;
    reply->deleteLater();
    releaseGatewayProbeWifiNetworkBinding();

    const bool oldCampusGatewayReachable = m_campusGatewayReachable;
    const bool oldCampusWifiDetected = m_campusWifiDetected;
    const QString oldStatus = m_networkStatusText;

    m_campusGatewayReachable = m_wifiConnected && reachable;
    m_campusWifiDetected = m_wifiConnected && (isCampusWifiSsid(m_currentSsid) || m_campusGatewayReachable);
    m_networkStatusText = buildNetworkStatusText();

    if (oldCampusGatewayReachable != m_campusGatewayReachable
        || oldCampusWifiDetected != m_campusWifiDetected
        || oldStatus != m_networkStatusText) {
        emit networkStateChanged();
    }

    evaluateAutoLoginSchedule();
}

void MobileBackend::clearGatewayProbe()
{
    if (!m_gatewayProbeReply) {
        return;
    }

    disconnect(m_gatewayProbeReply, nullptr, this, nullptr);
    m_gatewayProbeReply->abort();
    m_gatewayProbeReply->deleteLater();
    m_gatewayProbeReply = nullptr;
    releaseGatewayProbeWifiNetworkBinding();
}

bool MobileBackend::isCampusWifiSsid(const QString &ssid)
{
    const QString normalized = ssid.trimmed();
    return normalized.compare(QStringLiteral("AUST_Student"), Qt::CaseInsensitive) == 0
        || normalized.compare(QStringLiteral("AUST_Faculty"), Qt::CaseInsensitive) == 0
        || normalized.startsWith(QStringLiteral("AUST"), Qt::CaseInsensitive);
}

void MobileBackend::finishLogin()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    setBusy(false);
    releaseLoginWifiNetworkBinding();

    if (!reply) {
        m_currentLoginAutomatic = false;
        setStatusText(QStringLiteral("登录状态异常"));
        emit loginFailed(m_statusText);
        return;
    }

    const QNetworkReply::NetworkError error = reply->error();
    const QString errorText = reply->errorString();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString response = QString::fromUtf8(reply->readAll());
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        m_currentLoginAutomatic = false;
        setStatusText(QStringLiteral("登录请求失败：%1").arg(errorText));
        emit loginFailed(m_statusText);
        return;
    }

    if (!responseLooksSuccessful(response)) {
        if (scheduleAutoLoginRetry(statusCode)) {
            return;
        }
        m_currentLoginAutomatic = false;
        setStatusText(QStringLiteral("服务器响应异常，可能登录失败（HTTP %1）").arg(statusCode));
        emit loginFailed(m_statusText);
        return;
    }

    m_settings.setValue("lastLoginTime", QDateTime::currentDateTime());
    m_settings.sync();
    m_lastSuccessfulLogin = QDateTime::currentDateTime();
    m_autoLoginTransientRetryCount = 0;
    m_currentLoginAutomatic = false;

    const QString accountType = m_loginMode == LoginMode::Teacher ? QStringLiteral("教师") : QStringLiteral("学生");
    setStatusText(QStringLiteral("%1账号登录成功").arg(accountType));
    emit loginSucceeded(m_statusText);
}

bool MobileBackend::responseLooksSuccessful(const QString &response) const
{
    if (response.contains("\"result\":1")
        || response.contains("\"result\":\"1\"")
        || response.contains("login_ok", Qt::CaseInsensitive)
        || response.contains(QStringLiteral("登录成功"))
        || response.contains(QStringLiteral("认证成功"))) {
        return true;
    }
    if (response.contains("UID=") && response.contains("@jzg") && response.contains("ac0=")) {
        return true;
    }
    if (response.length() > 1000 && response.contains("<script", Qt::CaseInsensitive) && response.contains("UID=")) {
        return true;
    }
    if (response.contains("\"uid\":") && response.contains("@jzg") &&
        (response.contains("\"ss5\":") || response.contains("\"ss6\":"))) {
        return true;
    }
    return false;
}

void MobileBackend::clearReply()
{
    if (!m_reply) {
        return;
    }
    disconnect(m_reply, nullptr, this, nullptr);
    m_reply->deleteLater();
    m_reply = nullptr;
    releaseLoginWifiNetworkBinding();
}
