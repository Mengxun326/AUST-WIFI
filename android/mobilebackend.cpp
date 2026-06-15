#include "mobilebackend.h"

#include "credentialstore.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

namespace {
constexpr char kStudentLoginUrl[] = "http://10.255.0.19/a79.htm";
constexpr char kTeacherLoginUrl[] = "http://10.255.0.19/drcom/login";
constexpr int kNetworkRefreshIntervalMs = 15000;
constexpr int kAutoLoginCooldownSeconds = 45;

#ifdef Q_OS_ANDROID
constexpr char kAndroidNetworkHelperClass[] = "top/mengxun/austwifi/NetworkStateHelper";
constexpr char kAndroidForegroundServiceClass[] = "top/mengxun/austwifi/AustWifiForegroundService";

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
#endif
}

MobileBackend::MobileBackend(QObject *parent)
    : QObject(parent)
    , m_settings("AUST_WIFI", "Android")
{
    loadConfig();
    connect(&m_networkRefreshTimer, &QTimer::timeout, this, &MobileBackend::refreshNetworkState);
    m_networkRefreshTimer.start(kNetworkRefreshIntervalMs);
    QTimer::singleShot(400, this, &MobileBackend::refreshNetworkState);
    QTimer::singleShot(600, this, &MobileBackend::refreshNotificationPermission);
    QTimer::singleShot(900, this, &MobileBackend::syncBackgroundServiceState);
    QTimer::singleShot(1200, this, &MobileBackend::runStartupAutoLogin);
}

MobileBackend::~MobileBackend()
{
    cancelLogin();
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
    if (m_busy) {
        setStatusText(QStringLiteral("登录请求正在进行，请稍后"));
        return;
    }

    if (!saveConfig()) {
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
    emit loginFailed(m_statusText);
}

void MobileBackend::cancelLogin()
{
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
    const QString missingPermissions = object.value("missingPermissions").toString();
    const bool canReadSsid = object.value("canReadSsid").toBool(false);
    const bool campusWifiDetected = isCampusWifiSsid(ssid);

    QString status;
    if (!missingPermissions.isEmpty()) {
        status = QStringLiteral("需要授权%1后才能识别当前 WiFi").arg(missingPermissions);
    } else if (!wifiConnected) {
        status = QStringLiteral("当前未连接 WiFi");
    } else if (!canReadSsid) {
        status = QStringLiteral("已连接 WiFi，但系统未返回 SSID");
    } else if (campusWifiDetected) {
        status = QStringLiteral("已连接校园 WiFi：%1").arg(ssid);
    } else {
        status = QStringLiteral("当前 WiFi：%1").arg(ssid);
    }

    const bool changed = m_wifiConnected != wifiConnected
        || m_currentSsid != ssid
        || m_missingNetworkPermissions != missingPermissions
        || m_campusWifiDetected != campusWifiDetected
        || m_networkStatusText != status;

    m_wifiConnected = wifiConnected;
    m_currentSsid = ssid;
    m_missingNetworkPermissions = missingPermissions;
    m_campusWifiDetected = campusWifiDetected;
    m_networkStatusText = status;

    if (changed) {
        emit networkStateChanged();
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
        setStatusText(QStringLiteral("启动自动登录已开启，等待连接校园 WiFi"));
        return;
    }

    setStatusText(QStringLiteral("启动自动登录中..."));
    m_lastAutoLoginAttempt = QDateTime::currentDateTime();
    login();
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
    setStatusText(QStringLiteral("检测到可用 WiFi，正在自动登录..."));
    login();
}

void MobileBackend::syncBackgroundServiceState()
{
#ifdef Q_OS_ANDROID
    refreshNotificationPermission();
    if (m_backgroundServiceEnabled) {
        const bool started = callAndroidForegroundServiceMethod("start");
        setBackgroundServiceStatusText(started
            ? QStringLiteral("后台守护服务运行中")
            : QStringLiteral("后台守护服务启动失败，请检查通知权限"));
        if (!started) {
            m_backgroundServiceEnabled = false;
            m_settings.setValue("backgroundService/enabled", false);
            m_settings.sync();
            emit serviceStateChanged();
        }
        return;
    }

    callAndroidForegroundServiceMethod("stop");
    setBackgroundServiceStatusText(QStringLiteral("后台守护服务未开启"));
#else
    setBackgroundServiceStatusText(QStringLiteral("后台守护服务仅 Android 端可用"));
#endif
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

    if (!reply) {
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
        setStatusText(QStringLiteral("登录请求失败：%1").arg(errorText));
        emit loginFailed(m_statusText);
        return;
    }

    if (!responseLooksSuccessful(response)) {
        setStatusText(QStringLiteral("服务器响应异常，可能登录失败（HTTP %1）").arg(statusCode));
        emit loginFailed(m_statusText);
        return;
    }

    m_settings.setValue("lastLoginTime", QDateTime::currentDateTime());
    m_settings.sync();
    m_lastSuccessfulLogin = QDateTime::currentDateTime();

    const QString accountType = m_loginMode == LoginMode::Teacher ? QStringLiteral("教师") : QStringLiteral("学生");
    setStatusText(QStringLiteral("%1账号登录成功").arg(accountType));
    emit loginSucceeded(m_statusText);
}

bool MobileBackend::responseLooksSuccessful(const QString &response) const
{
    if (response.contains("\"result\":1") || response.contains("login_ok", Qt::CaseInsensitive)) {
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
}
