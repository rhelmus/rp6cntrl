TEMPLATE = app
TARGET = client-ncurses

DEPENDPATH += tui
HEADERS += \
    tui/basescroll.h \
    tui/box.h \
    tui/button.h \
    tui/buttonbar.h \
    tui/group.h \
    tui/label.h \
    tui/separator.h \
    tui/scrollbar.h \
    tui/textbase.h \
    tui/textfield.h \
    tui/textwidget.h \
    tui/tui.h \
    tui/utf8.h \
    tui/widget.h \
    tui/window.h \
    tui/windowmanager.h
SOURCES += \
    tui/basescroll.cpp \
    tui/box.cpp \
    tui/button.cpp \
    tui/buttonbar.cpp \
    tui/group.cpp \
    tui/label.cpp \
    tui/scrollbar.cpp \
    tui/textbase.cpp \
    tui/textfield.cpp \
    tui/textwidget.cpp \
    tui/tui.cpp \
    tui/utils.cpp \
    tui/widget.cpp \
    tui/window.cpp \
    tui/windowmanager.cpp
    
HEADERS += client.h display_widget.h
SOURCES += client.cpp display_widget.cpp main.cpp

QT -= gui
QT += network

LIBS += -lncursesw

INCLUDEPATH += ../../shared
DEPENDPATH += ../../shared
SOURCES += tcputil.cpp

INCLUDEPATH += ../client-base
DEPENDPATH += ../client-base
HEADERS += client_base.h
SOURCES += client_base.cpp

OBJECTS_DIR = obj