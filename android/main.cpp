#include "app_config.h"
#include "mobilebackend.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("AUST WiFi");
    QGuiApplication::setApplicationVersion(APP_VERSION);
    QGuiApplication::setOrganizationName("AUST");
    QGuiApplication::setOrganizationDomain("aust.edu.cn");

    QQuickStyle::setStyle("Material");

    MobileBackend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
