cmake_minimum_required(VERSION 3.14)
project(anoii-poll)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include(FetchContent)

# GoogleTest requires at least C++14
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 
# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )

#include(cmake/CPM.cmake)
#
#CPMAddPackage("gh:fmtlib/fmt#7.1.3")

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
)
FetchContent_MakeAvailable(fmt)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(examples)

# 添加一个选项，默认关闭 ClangFormat
option(ENABLE_CLANGFORMAT "Enable ClangFormat formatting" OFF)

if(ENABLE_CLANGFORMAT)
  include(ClangFormat)
  clangformat_setup(
    "src/*.cc"
    "src/include/*.h"
    "examples/*.cc"
    "tests/*.cc"
  )
  message(STATUS "ClangFormat enabled.")
else()
  message(STATUS "ClangFormat disabled.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_BUILD_TYPE Debug)
