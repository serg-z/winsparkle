QT       -= core gui

#CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = WinSparkle
TEMPLATE = lib
CONFIG += shared

include(../common.pri)

DEFINES += BUILDING_WIN_SPARKLE __WXMSW__ _UNICODE

WX_VERSION = 30

CONFIG(debug, debug|release) {
    DEFINES += NDEBUG
    WX_SUFFIX = d
}

LIBS += \
    -L$$PWD/3rdparty/wxWidgets/lib/gcc_lib \
    -lwxexpat$${WX_SUFFIX} \
    -lwxmsw$${WX_VERSION}u$${WX_SUFFIX}_core \
    -lwxbase$${WX_VERSION}u$${WX_SUFFIX} \
    -lwxpng$${WX_SUFFIX} \
    -lz \
    -lWinspool \
    -lComdlg32 \
    -lComctl32 \
    -lGdi32 \
    -lWininet \
    -lVersion \
    -luuid \
    -lOle32 \
    -lOleAut32 \
    -lRpcrt4

INCLUDEPATH += \
    include \
    3rdparty/wxWidgets/include \
    3rdparty/wxWidgets/lib/gcc_lib/mswud \
    3rdparty/expat/lib

HEADERS += \
    src/appcast.h \
    src/appcontroller.h \
    src/download.h \
    src/error.h \
    src/settings.h \
    src/threads.h \
    src/ui.h \
    src/updatechecker.h \
    src/updatedownloader.h \
    src/utils.h

SOURCES += \
    src/appcast.cpp \
    src/appcontroller.cpp \
    src/dll_api.cpp \
    src/dllmain.cpp \
    src/download.cpp \
    src/error.cpp \
    src/settings.cpp \
    src/threads.cpp \
    src/ui.cpp \
    src/updatechecker.cpp \
    src/updatedownloader.cpp
