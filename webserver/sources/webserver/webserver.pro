TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    base64.c

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    webserver.h \
    base64.h
QMAKE_CFLAGS += -std=c99 -Wno-format-security -Wno-format
LIBS += -L/home/konstantce/webserver -lrt -ldl -lmicrohttpd -lpthread -lsqlite3 -lchallenge
DESTDIR = /home/konstantce/iss/iss/Volga2015webserver/


