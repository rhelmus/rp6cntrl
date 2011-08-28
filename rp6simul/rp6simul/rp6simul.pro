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
    mapsettingsdialog.cpp \
    clock.cpp \
    progressdialog.cpp \
    robotwidget.cpp

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
    mapsettingsdialog.h \
    clock.h \
    graphicsitemtypes.h \
    progressdialog.h \
    robotwidget.h \
    led.h \
    bumper.h \
    irsensor.h \
    lightsensor.h

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
    lua/drivers/acs.lua \
    lua/properties.lua \
    lua/drivers/bumper.lua \
    lua/drivers/light.lua \
    lua/drivers/ircomm.lua

win32 {
    SOURCES += ../lua/lzio.c \
        ../lua/lvm.c \
        ../lua/lundump.c \
        ../lua/ltm.c \
        ../lua/ltablib.c \
        ../lua/ltable.c \
        ../lua/lstrlib.c \
        ../lua/lstring.c \
        ../lua/lstate.c \
        ../lua/lparser.c \
        ../lua/loslib.c \
        ../lua/lopcodes.c \
        ../lua/lobject.c \
        ../lua/loadlib.c \
        ../lua/lmem.c \
        ../lua/lmathlib.c \
        ../lua/llex.c \
        ../lua/liolib.c \
        ../lua/linit.c \
        ../lua/lgc.c \
        ../lua/lfunc.c \
        ../lua/ldump.c \
        ../lua/ldo.c \
        ../lua/ldebug.c \
        ../lua/ldblib.c \
        ../lua/lcode.c \
        ../lua/lbaselib.c \
        ../lua/lauxlib.c \
        ../lua/lapi.c

    HEADERS += ../lua/lzio.h \
        ../lua/lvm.h \
        ../lua/lundump.h \
        ../lua/lualib.h \
        ../lua/luaconf.h \
        ../lua/lua.h \
        ../lua/ltm.h \
        ../lua/ltable.h \
        ../lua/lstring.h \
        ../lua/lstate.h \
        ../lua/lparser.h \
        ../lua/lopcodes.h \
        ../lua/lobject.h \
        ../lua/lmem.h \
        ../lua/llimits.h \
        ../lua/llex.h \
        ../lua/lgc.h \
        ../lua/lfunc.h \
        ../lua/ldo.h \
        ../lua/ldebug.h \
        ../lua/lcode.h \
        ../lua/lauxlib.h \
        ../lua/lapi.h \
        ../lua/lua.hpp

    INCLUDEPATH += ../lua
}
else {
    LIBS += -L/mnt/stuff/shared/src/LuaJIT-2.0.0-beta8/prefix/lib -lluajit-5.1
#    LIBS += -llua
}
