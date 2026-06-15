#include "mobilebackend.h"

#include "credentialstore.h"

#include <QDateTime>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr char kStudentLoginUrl[] = "http://10.255.0.19/a79.htm";
constexpr char kTeacherLoginUrl[] = "http://10.255.0.19/drcom/login";
}

MobileBackend::MobileBackend(QObject *parent)
    : QObject(parent)
    , m_settings("AUST_WIFI", "Android")
{
    loadConfig();
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

bool MobileBackend::autoLoginOnLaunch() const
{
    return m_autoLoginOnLaunch;
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

    emit configChanged();
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

    setStatusText(QStringLiteral("启动自动登录中..."));
    login();
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
