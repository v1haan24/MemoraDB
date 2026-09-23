cmake_minimum_required(VERSION 3.20)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    test_main.cpp
    lexer/test_lexer.cpp
    parser/test_parser.cpp
    catalog/test_catalog.cpp
    storage/test_storage.cpp
    query/test_executor.cpp
    temporal/test_temporal.cpp
    index/test_index.cpp
    vector/test_vector.cpp
    integration/test_integration.cpp
)

add_executable(memora_tests ${TEST_SOURCES})

target_include_directories(memora_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(memora_tests PRIVATE memora GTest::gtest)

if(WIN32)
    add_custom_command(TARGET memora_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll" "$<TARGET_FILE_DIR:memora_tests>/onnxruntime.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_shared.dll" "$<TARGET_FILE_DIR:memora_tests>/onnxruntime_providers_shared.dll"
    )
else()
    add_custom_command(TARGET memora_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_LIBRARY}" "$<TARGET_FILE_DIR:memora_tests>"
    )
endif()

include(GoogleTest)
gtest_discover_tests(memora_tests)
