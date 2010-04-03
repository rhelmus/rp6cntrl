TEMPLATE = app
TARGET = client-qt
HEADERS += client.h \
 sensorplot.h \
 ../../shared/shared.h
SOURCES += main.cpp
QT += network

DEPENDPATH += ../../shared
SOURCES += tcputil.cpp \
 client.cpp \
 sensorplot.cpp
INCLUDEPATH += /usr/include/qwt/ \
  ../../shared

LIBS += -lqwt

