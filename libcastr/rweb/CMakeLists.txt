cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

set(SOURCES
    RWeb.cxx
    RWebUtils.cxx
)

include_directories( ./include )

add_library(rweb STATIC ${SOURCES})
target_link_libraries(rweb rlog)

target_include_directories(rweb INTERFACE 
                           ${CMAKE_CURRENT_SOURCE_DIR}/include )
