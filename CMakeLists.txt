cmake_minimum_required(VERSION 3.20)

project(MemoraDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# Python
# ------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)


# ------------------------------------------------------------
# Main MemoraDB library
# ------------------------------------------------------------

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# embedMake.cpp has its own main(), so it must NOT be part
# of the memora library.
list(FILTER SOURCES EXCLUDE REGEX ".*/vector/embedMake\\.cpp$")

add_library(memora ${SOURCES})

target_include_directories(memora PUBLIC src)


# ------------------------------------------------------------
# Normal C++ tests
# ------------------------------------------------------------

file(GLOB_RECURSE TESTS
    tests/*.cpp
)

# winprocess.cpp is a helper source, not a standalone test.
# test_live_pipeline_windows.cpp is handled separately below.
list(FILTER TESTS EXCLUDE REGEX ".*/winprocess\\.cpp$")
list(FILTER TESTS EXCLUDE REGEX ".*/test_live_pipeline_windows\\.cpp$")

foreach(test ${TESTS})

    get_filename_component(name ${test} NAME_WE)

    add_executable(${name} ${test})

    target_link_libraries(${name} PRIVATE memora)

endforeach()


# ------------------------------------------------------------
# embedMake executable
# ------------------------------------------------------------

add_executable(embedMake
    src/vector/embedMake.cpp
)

target_link_libraries(embedMake PRIVATE memora)


# ------------------------------------------------------------
# Windows live end-to-end test
# ------------------------------------------------------------

add_executable(test_live_pipeline_windows
    tests/test_live_pipeline_windows.cpp
    tests/winprocess.cpp
)

target_link_libraries(test_live_pipeline_windows PRIVATE memora)


# ------------------------------------------------------------
# CTest
# ------------------------------------------------------------

enable_testing()

add_test(
    NAME live_pipeline_windows
    COMMAND
        $<TARGET_FILE:test_live_pipeline_windows>
        $<TARGET_FILE:embedMake>
        ${CMAKE_SOURCE_DIR}/src/vector/ml_worker.py
        180000
        ${Python3_EXECUTABLE}
)

set_tests_properties(
    live_pipeline_windows
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# ------------------------------------------------------------
# One-command target
# ------------------------------------------------------------

add_custom_target(live_pipeline
    COMMAND
        ${CMAKE_CTEST_COMMAND}
        --test-dir ${CMAKE_BINARY_DIR}
        -R "^live_pipeline_windows$"
        --verbose

    DEPENDS
        embedMake
        test_live_pipeline_windows

    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)