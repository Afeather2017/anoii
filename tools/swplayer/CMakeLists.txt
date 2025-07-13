add_executable(swplayer main.cc)
target_link_libraries(swplayer anoii anoii_http)
target_compile_definitions(swplayer
                           PUBLIC SRC_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
