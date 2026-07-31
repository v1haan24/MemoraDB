cmake_minimum_required(VERSION 3.20)
project(MemoraDB LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#src-memoradb
file(GLOB_RECURSE MEMORA_SOURCES
    CONFIGURE_DEPENDS
    src/*.cpp
)
add_library(memora ${MEMORA_SOURCES})
target_include_directories(memora
    PUBLIC
        src
)

#tests
file(GLOB TEST_SOURCES
    CONFIGURE_DEPENDS
    tests/*.cpp
)
foreach(test_file ${TEST_SOURCES})
    get_filename_component(test_name
        ${test_file}
        NAME_WE
    )
    add_executable(${test_name}
        ${test_file}
    )
    target_link_libraries(${test_name}
        PRIVATE
            memora
    )
endforeach()

#benchmarks
file(GLOB BENCHMARK_SOURCES
    CONFIGURE_DEPENDS
    benchmark/*.cpp
)
foreach(bench_file ${BENCHMARK_SOURCES})
    get_filename_component(bench_name
        ${bench_file}
        NAME_WE
    )
    add_executable(${bench_name}
        ${bench_file}
    )
    target_link_libraries(${bench_name}
        PRIVATE
            memora
    )
endforeach()