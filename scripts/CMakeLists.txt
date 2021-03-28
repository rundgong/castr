
add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/castr-convert
    COMMAND ${CMAKE_COMMAND} -E copy 
            ${CMAKE_CURRENT_SOURCE_DIR}/castr-convert
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/castr-convert
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/castr-convert
            )

add_custom_target(copy-scripts ALL
    DEPENDS castr-convert
    )
