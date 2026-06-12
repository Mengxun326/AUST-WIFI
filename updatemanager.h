#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class QFile;

struct UpdateInfo
{
    QString version;
    QString minSupportedVersion;
    QString url;
    QString sha256;
    QString notes;
    QString publishedAt;
    bool force = false;
    qint64 size = 0;
};

Q_DECLARE_METATYPE(UpdateInfo)

class UpdateManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(QObject *parent = nullptr);
    ~UpdateManager();

    void checkForUpdates(bool manual);
    void downloadUpdate(const UpdateInfo &info);
    void cancelDownload();
    bool launchInstaller();
    bool isBusy() const;
    QString downloadedInstallerPath() const;

signals:
    void updateAvailable(const UpdateInfo &info, bool manual);
    void noUpdateAvailable(const QString &currentVersion, const QString &latestVersion, bool manual);
    void updateCheckFailed(const QString &message, bool manual);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &installerPath);
    void updateFailed(const QString &message);

private slots:
    void onManifestFinished();
    void onDownloadReadyRead();
    void onDownloadFinished();

private:
    UpdateInfo parseManifest(const QByteArray &data, QString *errorMessage) const;
    int compareVersions(const QString &left, const QString &right) const;
    bool verifySha256(const QString &filePath, const QString &expectedHash, QString *errorMessage) const;
    QString buildDownloadPath(const UpdateInfo &info) const;
    void clearDownloadState();

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_manifestReply;
    QNetworkReply *m_downloadReply;
    QFile *m_downloadFile;
    UpdateInfo m_pendingUpdate;
    QString m_downloadedInstallerPath;
    bool m_manualCheck;
};

#endif // UPDATEMANAGER_H
