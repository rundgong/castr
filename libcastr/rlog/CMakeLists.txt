cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

include_directories( ./include )

add_library(rlog OBJECT RLog.cxx)

target_include_directories(rlog INTERFACE 
                           ${CMAKE_CURRENT_SOURCE_DIR}/include )
