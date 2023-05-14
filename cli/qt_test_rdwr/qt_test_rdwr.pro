TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ./src/main.cpp \
        ./src/tostring.cpp

HEADERS += \
        ./src/tostring.h

DISTFILES += \
        ./src/note.txt

# StorageApi code
include(../../api.pri)
