TEMPLATE = app
TARGET = client-qt
HEADERS += client.h \
 sensorplot.h \
 ../../shared/shared.h \
 scanner.h \
 editor.h
SOURCES += main.cpp
QT += network

DEPENDPATH += ../../shared
SOURCES += tcputil.cpp \
 client.cpp \
 sensorplot.cpp \
 scanner.cpp \
 editor.cpp
INCLUDEPATH += /usr/include/qwt/ \
  ../../shared


CONFIG += qcodeedit

LIBS += -lqwt
RESOURCES += editor.qrc

