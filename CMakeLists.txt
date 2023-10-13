cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(coco VERSION 1.0.0 LANGUAGES C)
cmake_policy(SET CMP0076 NEW)
include(CTest)

add_library(coco STATIC)

set(problemChildren 
-fno-tree-forwprop
-fno-omit-frame-pointer
)
# https://gcc.gnu.org/onlinedocs/gcc/Unnamed-Fields.html
target_compile_options(coco PUBLIC -g -O3 ${problemChildren} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -Wall -Wextra -fplan9-extensions -std=c99)
target_link_options(coco PUBLIC ${sanitizers})
target_include_directories(coco PUBLIC src)
add_subdirectory(src)
install(TARGETS coco)

set(exs "example1_channels;\
example2_waitgroups;\
example3_signals;\
example4_nested_yield;\
example5_counter;\
example6_counter_nobuff;\
example7_counter_servicer;\
test7_counter_servicer")

foreach(ex IN LISTS exs)
add_executable(${ex} ./examples/${ex}.c)
target_link_libraries(${ex} coco)
add_test(NAME ${ex} COMMAND ./${ex})
endforeach()

add_custom_target(force COMMAND make clean && make)