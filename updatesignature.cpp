#include "updatesignature.h"
#include "app_config.h"

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {

#ifdef Q_OS_WIN
QByteArray reversed(const QByteArray &data)
{
    QByteArray copy = data;
    std::reverse(copy.begin(), copy.end());
    return copy;
}

bool appendStruct(QByteArray *target, const void *source, int size)
{
    if (!target || !source || size <= 0) {
        return false;
    }

    target->append(static_cast<const char *>(source), size);
    return true;
}

bool buildPublicKeyBlob(QByteArray *blob, QString *errorMessage)
{
    const QByteArray modulus = QByteArray::fromBase64(APP_UPDATE_SIGNATURE_MODULUS);
    const QByteArray exponent = QByteArray::fromBase64(APP_UPDATE_SIGNATURE_EXPONENT);
    if (modulus.isEmpty() || exponent.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "更新签名公钥配置无效。";
        }
        return false;
    }

    DWORD publicExponent = 0;
    for (unsigned char byte : exponent) {
        publicExponent = (publicExponent << 8) | byte;
    }

    PUBLICKEYSTRUC header;
    header.bType = PUBLICKEYBLOB;
    header.bVersion = CUR_BLOB_VERSION;
    header.reserved = 0;
    header.aiKeyAlg = CALG_RSA_KEYX;

    RSAPUBKEY rsaKey;
    rsaKey.magic = 0x31415352; // RSA1
    rsaKey.bitlen = static_cast<DWORD>(modulus.size() * 8);
    rsaKey.pubexp = publicExponent;

    blob->clear();
    appendStruct(blob, &header, sizeof(header));
    appendStruct(blob, &rsaKey, sizeof(rsaKey));
    blob->append(reversed(modulus));
    return true;
}
#endif

}

bool UpdateSignature::verifyPayload(const QByteArray &payload,
                                    const QByteArray &signature,
                                    QString *errorMessage)
{
    if (payload.isEmpty() || signature.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "更新清单缺少签名 payload 或 signature。";
        }
        return false;
    }

#ifndef Q_OS_WIN
    Q_UNUSED(payload)
    Q_UNUSED(signature)
    if (errorMessage) {
        *errorMessage = "当前平台暂不支持更新清单签名校验。";
    }
    return false;
#else
    QByteArray publicKeyBlob;
    if (!buildPublicKeyBlob(&publicKeyBlob, errorMessage)) {
        return false;
    }

    HCRYPTPROV provider = 0;
    HCRYPTKEY publicKey = 0;
    HCRYPTHASH hash = 0;

    auto cleanup = [&]() {
        if (hash) {
            CryptDestroyHash(hash);
        }
        if (publicKey) {
            CryptDestroyKey(publicKey);
        }
        if (provider) {
            CryptReleaseContext(provider, 0);
        }
    };

    if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (errorMessage) {
            *errorMessage = "无法初始化更新签名校验环境。";
        }
        cleanup();
        return false;
    }

    if (!CryptImportKey(provider,
                        reinterpret_cast<const BYTE *>(publicKeyBlob.constData()),
                        static_cast<DWORD>(publicKeyBlob.size()),
                        0,
                        0,
                        &publicKey)) {
        if (errorMessage) {
            *errorMessage = "无法导入更新签名公钥。";
        }
        cleanup();
        return false;
    }

    if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
        if (errorMessage) {
            *errorMessage = "无法创建更新签名哈希。";
        }
        cleanup();
        return false;
    }

    if (!CryptHashData(hash,
                       reinterpret_cast<const BYTE *>(payload.constData()),
                       static_cast<DWORD>(payload.size()),
                       0)) {
        if (errorMessage) {
            *errorMessage = "无法计算更新清单签名哈希。";
        }
        cleanup();
        return false;
    }

    const QByteArray cryptoApiSignature = reversed(signature);
    const BOOL verified = CryptVerifySignature(hash,
                                               reinterpret_cast<const BYTE *>(cryptoApiSignature.constData()),
                                               static_cast<DWORD>(cryptoApiSignature.size()),
                                               publicKey,
                                               nullptr,
                                               0);
    cleanup();

    if (!verified) {
        if (errorMessage) {
            *errorMessage = "更新清单签名校验失败，已拒绝本次更新。";
        }
        return false;
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
#endif
}
