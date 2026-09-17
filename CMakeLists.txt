cmake_minimum_required(VERSION 3.20)
project(MemoraDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(memora ${SOURCES})
target_include_directories(memora PUBLIC src)

file(GLOB_RECURSE TESTS tests/*.cpp)
foreach(test ${TESTS})
    get_filename_component(name ${test} NAME_WE)
    add_executable(${name} ${test})
    target_link_libraries(${name} PRIVATE memora)
endforeach()