cmake_minimum_required(VERSION 3.0)


find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS cast_channel.proto)

add_library(cast_channel OBJECT ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(cast_channel ${Protobuf_LIBRARIES})

target_include_directories(cast_channel INTERFACE 
                           ${CMAKE_CURRENT_BINARY_DIR} )

