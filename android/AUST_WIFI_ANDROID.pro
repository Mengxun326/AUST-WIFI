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

android {
    android_openssl_root = $$(ANDROID_OPENSSL_ROOT)
    !isEmpty(android_openssl_root) {
        android_openssl_pri = $$clean_path($$android_openssl_root/openssl.pri)
        exists($$android_openssl_pri): include($$android_openssl_pri)
    }
}

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
ANDROID_VERSION_CODE = 16
ANDROID_VERSION_NAME = 1.0.6

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/res/xml/aust_wifi_file_paths.xml \
    android/src/top/mengxun/austwifi/ApkUpdateHelper.java \
    android/src/top/mengxun/austwifi/AustWifiForegroundService.java \
    android/src/top/mengxun/austwifi/NetworkStateHelper.java \
    android/src/top/mengxun/austwifi/SecureCredentialStore.java \
    assets/logo.png \
    qml/Main.qml
