QT += widgets serialport
CONFIG += c++17

TARGET = MT7987ADownloadTools
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/BoardModel.cpp \
    src/BoardConfigDialog.cpp \
    src/DownloadCommand.cpp \
    src/DownloadProcess.cpp

HEADERS += \
    src/MainWindow.h \
    src/BoardModel.h \
    src/BoardConfigDialog.h \
    src/DownloadCommand.h \
    src/DownloadProcess.h

win32: CONFIG += windows
msvc: QMAKE_CXXFLAGS += /utf-8
