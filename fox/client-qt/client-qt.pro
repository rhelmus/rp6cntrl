TEMPLATE = app
TARGET = client-qt
HEADERS += client.h \
 sensorplot.h
SOURCES += main.cpp
QT += network

DEPENDPATH += ../../shared
SOURCES += tcputil.cpp \
 client.cpp \
 sensorplot.cpp
INCLUDEPATH += /usr/include/qwt/ \
  ../../shared

LIBS += -lqwt

