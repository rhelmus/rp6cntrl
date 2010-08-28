TEMPLATE = app
TARGET = server
HEADERS += serial.h \
    server.h \
    tcp.h \
    shared.h \
    lua.h \
    ../../shared/tcputil.h \
    luanav.h
SOURCES += serial.cpp \
    tcp.cpp \
    server.cpp \
    main.cpp \
    lua.cpp \
    ../../shared/pathengine.cpp \
    luanav.cpp

# GUI is requiered as server provides lua bindings for QVector2D
QT += gui
QT += network
INCLUDEPATH += ../qextserialport/include
LIBS += -lqextserialport
INCLUDEPATH += ../../shared
DEPENDPATH += ../../shared
SOURCES += tcputil.cpp
INCLUDEPATH += /usr/include/lua5.1
LIBS += -llua
OTHER_FILES += main.lua \
    main.lua \
    ../scripts/nav.lua \
    ../scripts/rotate.lua \
    robot.lua
