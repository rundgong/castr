cmake_minimum_required(VERSION 3.0)

set(SOURCES
    SslWrapper.cxx
    Utils.cxx
    getch/getch.c
    json/jsoncpp.cpp
    fmt/src/format.cc
    fmt/src/os.cc
)

set(UTIL_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/getch
    ${CMAKE_CURRENT_SOURCE_DIR}/include )

include_directories( ${UTIL_INCLUDE_DIRS} )

add_library(castr_utils OBJECT ${SOURCES})

target_include_directories(castr_utils INTERFACE ${UTIL_INCLUDE_DIRS} )
