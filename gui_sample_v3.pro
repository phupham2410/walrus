#-------------------------------------------------
#
# Project created by QtCreator 2023-03-28T21:03:29
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gui_sample_v3
TEMPLATE = app
DEFINES += STORAGE_API_IMPL

INCLUDEPATH += src/api
INCLUDEPATH += src/cmn
INCLUDEPATH += gui/gui_sample_v3

SOURCES +=\
    gui/gui_sample_v3/Main.cpp \
    gui/gui_sample_v3/MainWindow.cpp \
    gui/gui_sample_v3/Worker.cpp

HEADERS  += \
    gui/gui_sample_v3/MainWindow.h \
    gui/gui_sample_v3/Worker.h

#Code from StorageAPI project

SOURCES += \
    src/cmn/AtaBase.cpp \
    src/cmn/AtaCmd.cpp \
    src/cmn/AtaCmdDirect.cpp \
    src/cmn/AtaCmdScsi.cpp \
    src/cmn/CmdBase.cpp \
    src/cmn/CommonUtil.cpp \
    src/cmn/CoreData.cpp \
    src/cmn/CoreUtil.cpp \
    src/cmn/DeviceCmn.cpp \
    src/cmn/DeviceUtil.cpp \
    src/cmn/DeviveMgr.cpp \
    src/cmn/HexFrmt.cpp \
    src/cmn/ScsiBase.cpp \
    src/cmn/ScsiCmd.cpp \
    src/cmn/StringUtil.cpp \
    src/cmn/SystemUtil.cpp \
    src/api/StorageApi.cpp

DISTFILES += \
    src/cmn/AtaCode.def \
    src/cmn/AttrName.def \
    src/cmn/CmdError.def \
    src/cmn/IdentifyKey.def \
    src/cmn/ScsiCode.def

HEADERS += \
    src/api/StorageApi.h \
    src/api/StorageApiTest.h \
    src/api/StorageApiImpl.h \
    src/api/StorageApiCmn.h \
    src/api/StorageApiNvme.h \
    src/api/StorageApiSata.h \
    src/api/StorageApiScsi.h \
    src/cmn/AtaBase.h \
    src/cmn/AtaCmd.h \
    src/cmn/CmdBase.h \
    src/cmn/CommonUtil.h \
    src/cmn/CoreData.h \
    src/cmn/CoreUtil.h \
    src/cmn/DeviceUtil.h \
    src/cmn/ScsiBase.h \
    src/cmn/ScsiCmd.h \
    src/cmn/StdHeader.h \
    src/cmn/StdMacro.h \
    src/cmn/StringUtil.h \
    src/cmn/SysHeader.h \
    src/cmn/SystemUtil.h \
    src/cmn/DeviceMgr.h \
    src/cmn/HexFrmt.h
