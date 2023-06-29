cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(coco VERSION 1.0.0 LANGUAGES C)
cmake_policy(SET CMP0076 NEW)
include(CTest)

add_library(coco SHARED)

set(CMAKE_CXX_FLAGS "-g -O3 -D_FORTIFY_SOURCE=0")
if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -pedantic -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

target_compile_options(coco PUBLIC )
target_include_directories(coco PUBLIC src)
add_subdirectory(src)
install(TARGETS coco)

add_executable(example1_channels ./examples/example1_channels.c)
target_link_libraries(example1_channels coco)
add_test(NAME example1_channels COMMAND ./example1_channels)

add_executable(example2_waitgroups ./examples/example2_waitgroups.c)
target_link_libraries(example2_waitgroups coco)
add_test(NAME example2_waitgroups COMMAND ./example2_waitgroups)

add_executable(example3_signals ./examples/example3_signals.c)
target_link_libraries(example3_signals coco)
add_test(NAME example3_signals COMMAND ./example3_signals)

add_executable(example4_nested_yield ./examples/example4_nested_yield.c)
target_link_libraries(example4_nested_yield coco)
add_test(NAME example4_nested_yield COMMAND ./example4_nested_yield)
