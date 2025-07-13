#add_executable(http_test http.cc test.cc context.cc const.cc)
#target_link_libraries(http_test anoii)
#target_compile_definitions(http_test
#                           PUBLIC HTML_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/index.html")
add_library(anoii_http http_server.cc http_request.cc http_response.cc http_utils.cc)
target_link_libraries(anoii_http anoii)
target_include_directories(anoii PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(http_test test.cc)
target_link_libraries(http_test anoii anoii_http)
target_compile_definitions(http_test
                           PUBLIC HTML_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/index.html")
