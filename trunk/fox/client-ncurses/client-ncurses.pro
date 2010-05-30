TEMPLATE = app
TARGET = client-ncurses

DEPENDPATH += tui
HEADERS += box.h \
    tui/button.h \
    tui/buttonbar.h \
    tui/group.h \
    tui/label.h \
    tui/separator.h \
    tui/textbase.h \
    tui/tui.h \
    tui/utf8.h \
    tui/widget.h \
    tui/window.h \
    tui/windowmanager.h
SOURCES += box.cpp \
    tui/button.cpp \
    tui/buttonbar.cpp \
    tui/group.cpp \
    tui/label.cpp \
    tui/textbase.cpp \
    tui/tui.cpp \
    tui/utils.cpp \
    tui/widget.cpp \
    tui/window.cpp \
    tui/windowmanager.cpp
    
HEADERS += client.h
SOURCES += client.cpp main.cpp

QT -= gui
QT += network

LIBS += -lncursesw

INCLUDEPATH += ../../shared
DEPENDPATH += ../../shared

OBJECTS_DIR = obj