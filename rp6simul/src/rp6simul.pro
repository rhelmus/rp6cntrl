#-------------------------------------------------
#
# Project created by QtCreator 2011-03-30T17:07:08
#
#-------------------------------------------------

QT       += core gui

TARGET = rp6simul
TEMPLATE = app

include(../qextserialport/qextserialport.pri)

DEFINES += USEATOMIC
INCLUDEPATH += ../shared/
LIBS += -lSDL

!debug {
    DEFINES += NDEBUG
}

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
    robotwidget.cpp \
    dataplotwidget.cpp \
    rotatablepixmapgraphicsitem.cpp \
    preferencesdialog.cpp

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
    lightsensor.h \
    dataplotwidget.h \
    rotatablepixmapgraphicsitem.h \
    preferencesdialog.h

OTHER_FILES += \
    TODO.txt \
    lua/main.lua \
    lua/robot/drivers/motor.lua \
    lua/robot/drivers/led.lua \
    lua/robot/drivers/acs.lua \
    lua/robot/properties.lua \
    lua/robot/drivers/bumper.lua \
    lua/robot/drivers/light.lua \
    lua/robot/drivers/ircomm.lua \
    lua/robot/robot.lua \
    lua/utils.lua \
    lua/m32/m32.lua \
    lua/m32/properties.lua \
    lua/shared_drivers/uart.lua \
    lua/shared_drivers/timer2.lua \
    lua/shared_drivers/timer1.lua \
    lua/shared_drivers/timer0.lua \
    lua/shared_drivers/adc.lua \
    lua/shared_drivers/portlog.lua \
    lua/m32/drivers/spi.lua \
    lua/m32/drivers/led.lua \
    lua/shared_drivers/twi.lua \
    lua/robot/drivers/extint1.lua \
    lua/m32/drivers/extint1.lua \
    lua/m32/drivers/exteeprom.lua \
    lua/m32/drivers/piezo.lua \
    lua/m32/drivers/mic.lua \
    lua/m32/drivers/keypad.lua \
    ../release.py

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

    INCLUDEPATH += ../SDL/include
    LIBS += -L../SDL/lib

    CONFIG += qwt
}
else {
#    LIBS += -L/mnt/stuff/shared/src/LuaJIT-2.0.0-beta8/prefix/lib -lluajit-5.1
    LIBS += -llua
    INCLUDEPATH += /usr/include/qwt
    LIBS += -lqwt
}

isEmpty(PREFIX): PREFIX=/usr/local

dataDir = $${PREFIX}/share/$${TARGET}
binDir = $${PREFIX}/bin
srcDir = $${dataDir}/src

!isEmpty(INCLUDESOURCE) {
sources.files = $${HEADERS} $${SOURCES} rp6simul.pro
sources.path = $${srcDir}
INSTALLS += sources
}

resources.files = ../resource/*.png ../resource/*.jpg ../resource/*.wav ../resource/sources.txt
!isEmpty(INCLUDESOURCE):resources.files += ../resource/*.svg
resources.path = $${dataDir}/resource
INSTALLS += resources

# Seems every subdirectory has to be specified individually,
# as we cannot use a defined function here (doesn't seem to be able to
# change INSTALLS, either by return value or directly), * globbing includes
# svn files, and specifying each subdirectory manually places the files in the
# same directory.
luaSrc.files = lua/*.lua
luaSrc.path = $${srcDir}/lua
INSTALLS += luaSrc
luaSrcShared.files = lua/shared_drivers/*.lua
luaSrcShared.path = $${srcDir}/lua/shared_drivers
INSTALLS += luaSrcShared
luaSrcRobot.files = lua/robot/*.lua
luaSrcRobot.path = $${srcDir}/lua/robot
INSTALLS += luaSrcRobot
luaSrcRobotDrivers.files = lua/robot/drivers/*.lua
luaSrcRobotDrivers.path = $${srcDir}/lua/robot/drivers
INSTALLS += luaSrcRobotDrivers
luaSrcM32.files = lua/m32/*.lua
luaSrcM32.path = $${srcDir}/lua/m32
INSTALLS += luaSrcM32
luaSrcM32Drivers.files = lua/m32/drivers/*.lua
luaSrcM32Drivers.path = $${srcDir}/lua/m32/drivers
INSTALLS += luaSrcM32Drivers

mapTemplates.files = ../map_templates/*
mapTemplates.path = $${dataDir}/map_templates
INSTALLS += mapTemplates

target.path = $${binDir}
INSTALLS += target
