cmake_minimum_required(VERSION 3.14)

project(testdiskclone VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

include_directories(../../application/src/storage_api)
include_directories(../../application/src/storage_api/cmn)
include_directories(../../application/src/storage_api/api)
add_subdirectory(../../application/src/storage_api ./build EXCLUDE_FROM_ALL)


set(PROJECT_SOURCES
        main.cpp
)

add_executable(testdiskclone 
    ${PROJECT_SOURCES}
)

target_link_libraries (testdiskclone LINK_PUBLIC storage_api)
