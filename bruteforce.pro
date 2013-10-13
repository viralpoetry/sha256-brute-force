TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.c \
    sha256.c \
    base64.c

HEADERS += \
    sha256.h \
    base64.h

LIBS += -lpthread
