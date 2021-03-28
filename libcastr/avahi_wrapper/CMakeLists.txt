cmake_minimum_required(VERSION 3.0)

set(SOURCES
    AvahiWrapper.cxx
)


include_directories( ./include )

add_library(avahi_wrapper OBJECT ${SOURCES})
target_link_libraries(avahi_wrapper rlog)

target_include_directories(avahi_wrapper INTERFACE 
                           ${CMAKE_CURRENT_SOURCE_DIR}/include )
