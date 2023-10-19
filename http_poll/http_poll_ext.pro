TEMPLATE = app
CONFIG += console
CONFIG += c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        http_poll_ext_server.cpp \
        main.cpp

LIBS += -pthread

HEADERS += \
    http_poll_ext_server.h
