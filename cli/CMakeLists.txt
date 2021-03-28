cmake_minimum_required(VERSION 3.0)

include_directories(.)


set(SOURCES
    castr_cli.cxx
    CliMediaStatus.cxx
)



SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

add_executable(castr-cli ${SOURCES})
target_link_libraries(castr-cli castr )

# Give castr-cli a proper name.
# This workaround is needed because cmake can not handle
# two targets with the same name
set_target_properties(castr-cli
        PROPERTIES OUTPUT_NAME castr)
