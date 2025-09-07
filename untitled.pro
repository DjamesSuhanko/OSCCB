QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Você pode desabilitar APIs deprecated se quiser:
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# ===== Android (USAR templates gerados pelo "Create Templates") =====
ANDROID_PACKAGE_SOURCE_DIR = $$PWD/Android
ANDROID_MIN_SDK_VERSION = 28
ANDROID_TARGET_SDK_VERSION = 35
# ====================================================================

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    moderndial.cpp \
    oscclient.cpp \
    titledialog.cpp

HEADERS += \
    mainwindow.h \
    moderndial.h \
    oscclient.h \
    titledialog.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resourcesFile.qrc

# (Opcional) listar arquivos para aparecerem no Qt Creator
OTHER_FILES += \
    Android/AndroidManifest.xml

DISTFILES += \
    Android/AndroidManifest.xml \
    Android/build.gradle \
    Android/res/values/libs.xml \
    Android/res/xml/qtprovider_paths.xml \


