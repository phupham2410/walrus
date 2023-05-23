TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ./src/main.cpp\
        ./src/ref.cpp

# StorageApi code
include(../../api.pri)
