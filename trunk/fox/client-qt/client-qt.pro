TEMPLATE = app
TARGET = client-qt
HEADERS += client.h \
 sensorplot.h \
 ../../shared/shared.h \
 ../client-base/client_base.h \
 scanner.h \
 editor.h
SOURCES += main.cpp
QT += network

DEPENDPATH += ../../shared \
 ../client-base

SOURCES += tcputil.cpp \
 client.cpp \
 sensorplot.cpp \
 scanner.cpp \
 editor.cpp \
 client_base.cpp


CONFIG += qcodeedit

LIBS += -lqwt
RESOURCES += editor.qrc

INCLUDEPATH += ../client-base/ \
  /usr/include/qwt/ \
  ../../shared

