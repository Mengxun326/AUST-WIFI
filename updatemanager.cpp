#include "updatemanager.h"
#include "app_config.h"
#include "updatesignature.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_manifestReply(nullptr)
    , m_downloadReply(nullptr)
    , m_downloadFile(nullptr)
    , m_manualCheck(false)
{
    qRegisterMetaType<UpdateInfo>("UpdateInfo");
}

UpdateManager::~UpdateManager()
{
    if (m_manifestReply) {
        m_manifestReply->abort();
        m_manifestReply->deleteLater();
        m_manifestReply = nullptr;
    }
    clearDownloadState();
}

void UpdateManager::checkForUpdates(bool manual)
{
    if (isBusy()) {
        if (manual) {
            emit updateCheckFailed("已有更新任务正在进行，请稍后再试。", true);
        }
        return;
    }

    m_manualCheck = manual;
    QNetworkRequest request(QUrl(APP_UPDATE_MANIFEST_URL));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QString("AUST-WIFI/%1").arg(QCoreApplication::applicationVersion()));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(10000);

    m_manifestReply = m_networkManager->get(request);
    connect(m_manifestReply, &QNetworkReply::finished, this, &UpdateManager::onManifestFinished);
}

void UpdateManager::downloadUpdate(const UpdateInfo &info)
{
    if (isBusy()) {
        emit updateFailed("已有更新任务正在进行，请稍后再试。");
        return;
    }

    m_pendingUpdate = info;
    m_downloadedInstallerPath.clear();

    const QString targetPath = buildDownloadPath(info);
    QDir().mkpath(QFileInfo(targetPath).absolutePath());

    m_downloadFile = new QFile(targetPath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        const QString error = QString("无法写入更新文件：%1").arg(m_downloadFile->errorString());
        clearDownloadState();
        emit updateFailed(error);
        return;
    }

    QNetworkRequest request(QUrl(info.url));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QString("AUST-WIFI/%1").arg(QCoreApplication::applicationVersion()));
    request.setTransferTimeout(60000);

    m_downloadReply = m_networkManager->get(request);
    connect(m_downloadReply, &QNetworkReply::readyRead, this, &UpdateManager::onDownloadReadyRead);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateManager::onDownloadFinished);
}

void UpdateManager::cancelDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
    }
}

bool UpdateManager::launchInstaller()
{
    if (m_downloadedInstallerPath.isEmpty() || !QFile::exists(m_downloadedInstallerPath)) {
        return false;
    }

    QStringList args;
#ifdef Q_OS_WIN
    args << "/SILENT" << "/SUPPRESSMSGBOXES" << "/NORESTART" << "/SP-";
#endif
    return QProcess::startDetached(m_downloadedInstallerPath, args);
}

bool UpdateManager::isBusy() const
{
    return m_manifestReply || m_downloadReply;
}

QString UpdateManager::downloadedInstallerPath() const
{
    return m_downloadedInstallerPath;
}

void UpdateManager::onManifestFinished()
{
    QNetworkReply *reply = m_manifestReply;
    m_manifestReply = nullptr;

    const bool manual = m_manualCheck;
    if (!reply) {
        emit updateCheckFailed("更新检查请求不存在。", manual);
        return;
    }

    const QNetworkReply::NetworkError error = reply->error();
    const QByteArray data = reply->readAll();
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        emit updateCheckFailed(QString("无法获取更新信息：%1").arg(errorText), manual);
        return;
    }

    QString parseError;
    const UpdateInfo info = parseManifest(data, &parseError);
    if (!parseError.isEmpty()) {
        emit updateCheckFailed(parseError, manual);
        return;
    }

    const QString currentVersion = QCoreApplication::applicationVersion();
    if (compareVersions(info.version, currentVersion) > 0) {
        emit updateAvailable(info, manual);
    } else {
        emit noUpdateAvailable(currentVersion, info.version, manual);
    }
}

void UpdateManager::onDownloadReadyRead()
{
    if (m_downloadReply && m_downloadFile) {
        m_downloadFile->write(m_downloadReply->readAll());
    }
}

void UpdateManager::onDownloadFinished()
{
    QNetworkReply *reply = m_downloadReply;
    QFile *file = m_downloadFile;
    m_downloadReply = nullptr;
    m_downloadFile = nullptr;

    if (!reply || !file) {
        emit updateFailed("更新下载状态异常。");
        return;
    }

    if (reply->bytesAvailable() > 0) {
        file->write(reply->readAll());
    }

    const QNetworkReply::NetworkError error = reply->error();
    const QString errorText = reply->errorString();
    const QString filePath = file->fileName();
    file->flush();
    file->close();
    file->deleteLater();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        QFile::remove(filePath);
        emit updateFailed(QString("更新下载失败：%1").arg(errorText));
        return;
    }

    QString hashError;
    if (!verifySha256(filePath, m_pendingUpdate.sha256, &hashError)) {
        QFile::remove(filePath);
        emit updateFailed(hashError);
        return;
    }

    m_downloadedInstallerPath = filePath;
    emit downloadFinished(filePath);
}

UpdateInfo UpdateManager::parseManifest(const QByteArray &data, QString *errorMessage) const
{
    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QString("更新清单格式错误：%1").arg(jsonError.errorString());
        }
        return {};
    }

    QJsonObject object = document.object();
    const QString payloadBase64 = object.value("payload").toString().trimmed();
    const QString signatureBase64 = object.value("signature").toString().trimmed();

#if APP_REQUIRE_SIGNED_MANIFEST
    if (payloadBase64.isEmpty() || signatureBase64.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "更新清单未签名，已拒绝本次更新。";
        }
        return {};
    }
#endif

    if (!payloadBase64.isEmpty() || !signatureBase64.isEmpty()) {
        const QByteArray payload = QByteArray::fromBase64(payloadBase64.toLatin1());
        const QByteArray signature = QByteArray::fromBase64(signatureBase64.toLatin1());
        QString signatureError;
        if (!UpdateSignature::verifyPayload(payload, signature, &signatureError)) {
            if (errorMessage) {
                *errorMessage = signatureError;
            }
            return {};
        }

        QJsonParseError payloadError;
        const QJsonDocument payloadDocument = QJsonDocument::fromJson(payload, &payloadError);
        if (payloadError.error != QJsonParseError::NoError || !payloadDocument.isObject()) {
            if (errorMessage) {
                *errorMessage = QString("更新清单签名 payload 格式错误：%1").arg(payloadError.errorString());
            }
            return {};
        }
        object = payloadDocument.object();
    }

    UpdateInfo info;
    info.version = object.value("latest").toString(object.value("version").toString()).trimmed();
    info.minSupportedVersion = object.value("min_supported").toString().trimmed();
    info.url = object.value("url").toString().trimmed();
    info.sha256 = object.value("sha256").toString().trimmed();
    info.notes = object.value("notes").toString().trimmed();
    info.publishedAt = object.value("published_at").toString().trimmed();
    info.force = object.value("force").toBool(false);
    info.size = static_cast<qint64>(object.value("size").toDouble(0));

    if (info.version.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "更新清单缺少 latest/version 字段。";
        }
        return {};
    }

    if (info.url.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "更新清单缺少安装包 url 字段。";
        }
        return {};
    }

    if (info.sha256.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "更新清单缺少 sha256 字段，无法安全更新。";
        }
        return {};
    }

    const QUrl packageUrl(info.url);
    if (!packageUrl.isValid() || packageUrl.scheme() != "https") {
        if (errorMessage) {
            *errorMessage = "更新包 URL 必须是有效的 HTTPS 地址。";
        }
        return {};
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return info;
}

int UpdateManager::compareVersions(const QString &left, const QString &right) const
{
    auto partsFor = [](QString version) {
        version = version.trimmed();
        if (version.startsWith('v', Qt::CaseInsensitive)) {
            version.remove(0, 1);
        }
        const QStringList rawParts = version.split(QRegularExpression("[^0-9]+"), Qt::SkipEmptyParts);
        QList<int> parts;
        for (const QString &part : rawParts) {
            parts.append(part.toInt());
        }
        return parts;
    };

    const QList<int> leftParts = partsFor(left);
    const QList<int> rightParts = partsFor(right);
    const int maxCount = qMax(leftParts.size(), rightParts.size());
    for (int i = 0; i < maxCount; ++i) {
        const int leftValue = i < leftParts.size() ? leftParts.at(i) : 0;
        const int rightValue = i < rightParts.size() ? rightParts.at(i) : 0;
        if (leftValue > rightValue) {
            return 1;
        }
        if (leftValue < rightValue) {
            return -1;
        }
    }
    return 0;
}

bool UpdateManager::verifySha256(const QString &filePath, const QString &expectedHash, QString *errorMessage) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QString("无法读取更新文件：%1").arg(file.errorString());
        }
        return false;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        hash.addData(file.read(1024 * 1024));
    }

    const QString actualHash = QString::fromLatin1(hash.result().toHex());
    if (actualHash.compare(expectedHash.trimmed(), Qt::CaseInsensitive) != 0) {
        if (errorMessage) {
            *errorMessage = QString("更新包校验失败。\n期望：%1\n实际：%2").arg(expectedHash, actualHash);
        }
        return false;
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

QString UpdateManager::buildDownloadPath(const UpdateInfo &info) const
{
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDir.isEmpty()) {
        tempDir = QDir::tempPath();
    }

    const QString version = info.version.trimmed().isEmpty() ? "latest" : info.version.trimmed();
    return QDir(tempDir).filePath(QString("AUST_WIFI_Update/AUST-WIFI-Setup-%1.exe").arg(version));
}

void UpdateManager::clearDownloadState()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }

    if (m_downloadFile) {
        const QString path = m_downloadFile->fileName();
        m_downloadFile->close();
        m_downloadFile->deleteLater();
        QFile::remove(path);
        m_downloadFile = nullptr;
    }
}
