#INCLUDEPATH += /mnt/stuff/shared/src/android/lighthouse/android-qt-mobility/install/include \
#    /mnt/stuff/shared/src/android/lighthouse/android-qt-mobility/install/include/QtMobility
CONFIG += mobility
MOBILITY += sensors

QT = core \
    gui \
    network \
    android
SOURCES = main.cpp \
    client.cpp \
    statwidget.cpp \
    flickcharm/flickcharm.cpp \
    drivewidget.cpp
CONFIG += dll

# "lib" and ".so" will be added automatically by qmake
TARGET = ../libs/armeabi/client-qt-android
HEADERS += client.h \
    statwidget.h \
    flickcharm/flickcharm.h \
    drivewidget.h
INCLUDEPATH += ../../../shared
DEPENDPATH += ../../../shared
SOURCES += tcputil.cpp
INCLUDEPATH += ../../client-base
DEPENDPATH += ../../client-base
HEADERS += client_base.h
SOURCES += client_base.cpp