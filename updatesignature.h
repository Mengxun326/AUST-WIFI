#ifndef UPDATESIGNATURE_H
#define UPDATESIGNATURE_H

#include <QByteArray>
#include <QString>

class UpdateSignature
{
public:
    static bool verifyPayload(const QByteArray &payload,
                              const QByteArray &signature,
                              QString *errorMessage);
};

#endif // UPDATESIGNATURE_H
