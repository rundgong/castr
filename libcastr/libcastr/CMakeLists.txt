cmake_minimum_required(VERSION 3.0)

add_library(castr STATIC )

target_link_libraries(castr cast_media_player cast_channel avahi_wrapper 
                            castr_utils rlog rweb
                            ssl crypto dl  protobuf protobuf-lite
                            avahi-client avahi-common pthread)

