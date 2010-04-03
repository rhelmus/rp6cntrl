TEMPLATE = app
TARGET = server
HEADERS += serial.h server.h tcp.h shared.h
SOURCES += serial.cpp tcp.cpp server.cpp main.cpp
QT -= gui
QT += network

INCLUDEPATH += ../qextserialport/include
LIBS += -lqextserialport

INCLUDEPATH += ../../shared
DEPENDPATH += ../../shared
SOURCES += tcputil.cpp