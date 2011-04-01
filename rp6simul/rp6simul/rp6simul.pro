#-------------------------------------------------
#
# Project created by QtCreator 2011-03-30T17:07:08
#
#-------------------------------------------------

QT       += core gui

TARGET = rp6simul
TEMPLATE = app

INCLUDEPATH += ../shared/

SOURCES += main.cpp\
        rp6simul.cpp \
    pluginthread.cpp \
    iohandler.cpp \
    avrtimer.cpp

HEADERS  += rp6simul.h \
    pluginthread.h \
    ../shared/shared.h \
    iohandler.h \
    avrtimer.h

OTHER_FILES += \
    TODO.txt
