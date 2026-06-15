#include "credentialstore.h"

#include <QByteArray>
#include <QSettings>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {
#ifdef Q_OS_ANDROID
constexpr char kAndroidCredentialStoreClass[] = "top/mengxun/austwifi/SecureCredentialStore";

QString callAndroidCredentialMethod(const char *methodName, const QString &scope, const QString &value)
{
    const QJniObject scopeArg = QJniObject::fromString(scope);
    const QJniObject valueArg = QJniObject::fromString(value);
    const QJniObject result = QJniObject::callStaticObjectMethod(
        kAndroidCredentialStoreClass,
        methodName,
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        scopeArg.object<jstring>(),
        valueArg.object<jstring>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent) || !result.isValid()) {
        return {};
    }

    return result.toString();
}
#endif
}

CredentialStore::CredentialStore(QSettings *settings)
    : m_settings(settings)
{
}

bool CredentialStore::setPassword(const QString &scope, const QString &password) const
{
    if (!m_settings) {
        return false;
    }

    const QString legacyKey = keyFor(scope, "password");
    const QString protectedKey = keyFor(scope, "password_dpapi");
    const QString storageKey = keyFor(scope, "password_storage");

    if (password.isEmpty()) {
        m_settings->remove(legacyKey);
        m_settings->remove(protectedKey);
        m_settings->remove(storageKey);
        return true;
    }

    QString protectedValue;
    if (protectPassword(scope, password, &protectedValue)) {
        m_settings->setValue(protectedKey, protectedValue);
        m_settings->setValue(storageKey, backendName());
        m_settings->remove(legacyKey);
        return true;
    }

#ifdef Q_OS_ANDROID
    return false;
#else
#ifndef Q_OS_WIN
    m_settings->setValue(legacyKey, password);
    m_settings->setValue(storageKey, backendName());
    m_settings->remove(protectedKey);
    return true;
#else
    return false;
#endif
#endif
}

QString CredentialStore::password(const QString &scope) const
{
    if (!m_settings) {
        return {};
    }

    const QString legacyKey = keyFor(scope, "password");
    const QString protectedKey = keyFor(scope, "password_dpapi");

    const QString protectedValue = m_settings->value(protectedKey).toString();
    if (!protectedValue.isEmpty()) {
        QString decrypted;
        if (unprotectPassword(scope, protectedValue, &decrypted)) {
            return decrypted;
        }
    }

    const QString legacyPassword = m_settings->value(legacyKey).toString();
    if (!legacyPassword.isEmpty() && secureBackendAvailable()) {
        setPassword(scope, legacyPassword);
    }
    return legacyPassword;
}

bool CredentialStore::hasPassword(const QString &scope) const
{
    return !password(scope).isEmpty();
}

void CredentialStore::migrateLegacyPassword(const QString &scope) const
{
    if (!m_settings || !secureBackendAvailable()) {
        return;
    }

    const QString legacyKey = keyFor(scope, "password");
    const QString protectedKey = keyFor(scope, "password_dpapi");
    if (!m_settings->value(protectedKey).toString().isEmpty()) {
        return;
    }

    const QString legacyPassword = m_settings->value(legacyKey).toString();
    if (!legacyPassword.isEmpty()) {
        setPassword(scope, legacyPassword);
    }
}

QString CredentialStore::backendName()
{
#ifdef Q_OS_ANDROID
    return "Android Keystore";
#else
#ifdef Q_OS_WIN
    return "Windows DPAPI";
#else
    return "QSettings fallback";
#endif
#endif
}

bool CredentialStore::secureBackendAvailable()
{
#ifdef Q_OS_ANDROID
    return true;
#else
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
#endif
}

QString CredentialStore::keyFor(const QString &scope, const QString &name)
{
    const QString trimmedScope = scope.trimmed();
    return trimmedScope.isEmpty() ? name : QString("%1/%2").arg(trimmedScope, name);
}

QByteArray CredentialStore::entropyFor(const QString &scope)
{
    return QString("AUST-WIFI credential v1:%1").arg(scope.trimmed()).toUtf8();
}

bool CredentialStore::protectPassword(const QString &scope, const QString &password, QString *protectedValue)
{
#ifdef Q_OS_ANDROID
    const QString encrypted = callAndroidCredentialMethod("encrypt", scope, password);
    if (encrypted.isEmpty()) {
        return false;
    }
    if (protectedValue) {
        *protectedValue = encrypted;
    }
    return true;
#else
#ifndef Q_OS_WIN
    Q_UNUSED(scope)
    Q_UNUSED(password)
    Q_UNUSED(protectedValue)
    return false;
#else
    const QByteArray plainBytes = password.toUtf8();
    QByteArray entropyBytes = entropyFor(scope);

    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plainBytes.constData()));
    input.cbData = static_cast<DWORD>(plainBytes.size());

    DATA_BLOB entropy;
    entropy.pbData = reinterpret_cast<BYTE *>(entropyBytes.data());
    entropy.cbData = static_cast<DWORD>(entropyBytes.size());

    DATA_BLOB output;
    output.pbData = nullptr;
    output.cbData = 0;

    const BOOL ok = CryptProtectData(&input,
                                     L"AUST-WIFI credential",
                                     &entropy,
                                     nullptr,
                                     nullptr,
                                     0,
                                     &output);
    if (!ok) {
        return false;
    }

    const QByteArray protectedBytes(reinterpret_cast<const char *>(output.pbData),
                                    static_cast<int>(output.cbData));
    LocalFree(output.pbData);

    if (protectedValue) {
        *protectedValue = QString::fromLatin1("dpapi:%1")
                              .arg(QString::fromLatin1(protectedBytes.toBase64()));
    }
    return true;
#endif
#endif
}

bool CredentialStore::unprotectPassword(const QString &scope, const QString &protectedValue, QString *password)
{
#ifdef Q_OS_ANDROID
    const QString decrypted = callAndroidCredentialMethod("decrypt", scope, protectedValue);
    if (decrypted.isEmpty()) {
        return false;
    }
    if (password) {
        *password = decrypted;
    }
    return true;
#else
#ifndef Q_OS_WIN
    Q_UNUSED(scope)
    Q_UNUSED(protectedValue)
    Q_UNUSED(password)
    return false;
#else
    QString encoded = protectedValue.trimmed();
    if (encoded.startsWith("dpapi:", Qt::CaseInsensitive)) {
        encoded = encoded.mid(6);
    }

    const QByteArray protectedBytes = QByteArray::fromBase64(encoded.toLatin1());
    if (protectedBytes.isEmpty()) {
        return false;
    }

    QByteArray entropyBytes = entropyFor(scope);
    DATA_BLOB input;
    input.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(protectedBytes.constData()));
    input.cbData = static_cast<DWORD>(protectedBytes.size());

    DATA_BLOB entropy;
    entropy.pbData = reinterpret_cast<BYTE *>(entropyBytes.data());
    entropy.cbData = static_cast<DWORD>(entropyBytes.size());

    DATA_BLOB output;
    output.pbData = nullptr;
    output.cbData = 0;

    const BOOL ok = CryptUnprotectData(&input,
                                       nullptr,
                                       &entropy,
                                       nullptr,
                                       nullptr,
                                       0,
                                       &output);
    if (!ok) {
        return false;
    }

    const QByteArray plainBytes(reinterpret_cast<const char *>(output.pbData),
                                static_cast<int>(output.cbData));
    LocalFree(output.pbData);

    if (password) {
        *password = QString::fromUtf8(plainBytes);
    }
    return true;
#endif
#endif
}
