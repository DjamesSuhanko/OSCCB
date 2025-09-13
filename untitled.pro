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
    channelmeterwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    meterswidget.cpp \
    modernbutton.cpp \
    moderncombobox.cpp \
    moderndial.cpp \
    modernprogressbar.cpp \
    oscclient.cpp \
    percentbarwidget.cpp \
    titledialog.cpp

HEADERS += \
    channelmeterwidget.h \
    mainwindow.h \
    meterswidget.h \
    modernbutton.h \
    moderncombobox.h \
    moderndial.h \
    modernprogressbar.h \
    oscclient.h \
    percentbarwidget.h \
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


