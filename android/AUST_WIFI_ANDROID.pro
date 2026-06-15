QT += core gui network qml quick quickcontrols2

CONFIG += c++17

TARGET = AUST_WIFI_ANDROID
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += \
    $$PWD/..

SOURCES += \
    main.cpp \
    mobilebackend.cpp \
    ../credentialstore.cpp

HEADERS += \
    mobilebackend.h \
    ../app_config.h \
    ../credentialstore.h

RESOURCES += \
    qml.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
ANDROID_VERSION_CODE = 5
ANDROID_VERSION_NAME = 0.5.0

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/src/top/mengxun/austwifi/AustWifiForegroundService.java \
    android/src/top/mengxun/austwifi/NetworkStateHelper.java \
    android/src/top/mengxun/austwifi/SecureCredentialStore.java \
    assets/logo.png \
    qml/Main.qml
