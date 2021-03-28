cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

set(SOURCES
    CastLink.cxx
    CastPayloads.cxx
    CastMediaPlayer.cxx
    ReceiverHandler.cxx
    MediaHandler.cxx
    StreamHelper.cxx
)



include_directories(./include)

add_library(cast_media_player OBJECT ${SOURCES})

target_link_libraries(cast_media_player
                      cast_channel
                      rlog
                      rweb
                      castr_utils)

target_include_directories(cast_media_player INTERFACE
                           ${CMAKE_CURRENT_SOURCE_DIR}/include )
