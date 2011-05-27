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
    avrtimer.cpp \
    lua.cpp \
    projectwizard.cpp \
    pathinput.cpp \
    projectsettings.cpp \
    utils.cpp \
    simulator.cpp \
    robotgraphicsitem.cpp \
    robotscene.cpp \
    handlegraphicsitem.cpp \
    resizablepixmapgraphicsitem.cpp \
    lightgraphicsitem.cpp \
    basegraphicsitem.cpp \
    mapsettingsdialog.cpp

HEADERS  += rp6simul.h \
    pluginthread.h \
    ../shared/shared.h \
    avrtimer.h \
    lua.h \
    projectwizard.h \
    pathinput.h \
    projectsettings.h \
    utils.h \
    simulator.h \
    robotgraphicsitem.h \
    robotscene.h \
    handlegraphicsitem.h \
    resizablepixmapgraphicsitem.h \
    lightgraphicsitem.h \
    basegraphicsitem.h \
    mapsettingsdialog.h

OTHER_FILES += \
    TODO.txt \
    lua/main.lua \
    lua/drivers/timer0.lua \
    lua/drivers/timer2.lua \
    lua/drivers/timer1.lua \
    lua/drivers/motor.lua \
    lua/drivers/uart.lua \
    lua/drivers/portlog.lua \
    lua/drivers/led.lua \
    lua/drivers/adc.lua \
    lua/drivers/acs.lua

INCLUDEPATH += /usr/include/lua5.1
LIBS += -llua
