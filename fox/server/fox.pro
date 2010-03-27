TEMPLATE = app
TARGET = fox
HEADERS += serial.h server.h
SOURCES += serial.cpp server.cpp main.cpp
QT -= gui 
INCLUDEPATH += ../qextserialport/include
LIBS += -L/usr/local/lib -lqextserialport