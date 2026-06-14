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
ANDROID_VERSION_CODE = 1
ANDROID_VERSION_NAME = 0.1.0

OTHER_FILES += \
    android/AndroidManifest.xml \
    qml/Main.qml
