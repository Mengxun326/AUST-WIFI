QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 默认使用Qt网络模块，跨平台可用
DEFINES += USE_QT_NETWORK

# 调试输出控制
# 如需重新启用调试输出，请注释掉下面两行
DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_INFO_OUTPUT

# 生产环境配置：禁用控制台窗口（仅Windows）
win32 {
    CONFIG += windows
    CONFIG -= console
    LIBS += -lcrypt32 -ladvapi32

    # 应用程序图标（仅Windows）
    RC_ICONS = icons/app.ico

    # libcurl配置（可选）
    exists(curl/include/curl/curl.h) {
        INCLUDEPATH += curl/include
        LIBS += -Lcurl/lib -lcurl
        DEFINES += USE_LIBCURL
        DEFINES -= USE_QT_NETWORK
    }
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    wifimanager.cpp \
    config_dialog.cpp \
    credentialstore.cpp \
    updatemanager.cpp \
    updatesignature.cpp

HEADERS += \
    app_config.h \
    mainwindow.h \
    wifimanager.h \
    config_dialog.h \
    credentialstore.h \
    updatemanager.h \
    updatesignature.h

FORMS += \
    mainwindow.ui \
    config_dialog.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
