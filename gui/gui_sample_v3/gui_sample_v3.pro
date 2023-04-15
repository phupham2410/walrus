#-------------------------------------------------
#
# Project created by QtCreator 2023-03-28T21:03:29
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gui_sample_v3
TEMPLATE = app
#DEFINES += STORAGE_API_TEST
DEFINES += STORAGE_API_IMPL

INCLUDEPATH += ./src

SOURCES +=\
    ./src/Main.cpp \
    ./src/MainWindow.cpp \
    ./src/Worker.cpp

HEADERS  += \
    ./src/MainWindow.h \
    ./src/Worker.h

# StorageApi code
include(../../api.pri)
