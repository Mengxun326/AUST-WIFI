#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <QString>

class QSettings;

class CredentialStore
{
public:
    explicit CredentialStore(QSettings *settings);

    bool setPassword(const QString &scope, const QString &password) const;
    QString password(const QString &scope) const;
    bool hasPassword(const QString &scope) const;
    void migrateLegacyPassword(const QString &scope) const;

    static QString backendName();
    static bool secureBackendAvailable();

private:
    static QString keyFor(const QString &scope, const QString &name);
    static QByteArray entropyFor(const QString &scope);
    static bool protectPassword(const QString &scope, const QString &password, QString *protectedValue);
    static bool unprotectPassword(const QString &scope, const QString &protectedValue, QString *password);

    QSettings *m_settings;
};

#endif // CREDENTIALSTORE_H
