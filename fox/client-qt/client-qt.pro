TEMPLATE = app
TARGET = client-qt
HEADERS += client.h
SOURCES += main.cpp
QT += network

INCLUDEPATH += ../../shared
DEPENDPATH += ../../shared
SOURCES += tcputil.cpp \
 client.cpp
