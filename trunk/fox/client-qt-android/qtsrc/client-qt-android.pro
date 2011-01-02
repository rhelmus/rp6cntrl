QT = core \
    gui \
    network \
    android
SOURCES = main.cpp \
    client.cpp \
    statwidget.cpp
CONFIG += dll

# "lib" and ".so" will be added automatically by qmake
TARGET = ../libs/armeabi/client-qt-android
HEADERS += client.h \
    statwidget.h
INCLUDEPATH += ../../../shared
DEPENDPATH += ../../../shared
SOURCES += tcputil.cpp
INCLUDEPATH += ../../client-base
DEPENDPATH += ../../client-base
HEADERS += client_base.h
SOURCES += client_base.cpp
