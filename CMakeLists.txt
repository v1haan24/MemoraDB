cmake_minimum_required(VERSION 3.20)

project(MemoraDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)


# ============================================================
# ONNX RUNTIME
# ============================================================

set(ONNXRUNTIME_VERSION "1.22.0" CACHE STRING "ONNX Runtime version")

if(NOT ONNXRUNTIME_ROOT)

    if(WIN32)

        if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|amd64|x86_64")
            message(FATAL_ERROR
                "Automatic ONNX Runtime download currently supports Windows x64 only."
            )
        endif()

        set(_ort_archive
            "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip"
        )

        set(_ort_url
            "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_ort_archive}"
        )

    elseif(APPLE)

        set(_ort_archive
            "onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}.tgz"
        )

        set(_ort_url
            "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_ort_archive}"
        )

    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux"
           AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")

        set(_ort_archive
            "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz"
        )

        set(_ort_url
            "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_ort_archive}"
        )

    else()

        message(FATAL_ERROR
            "Unsupported platform. Set ONNXRUNTIME_ROOT manually."
        )

    endif()

    FetchContent_Declare(
        onnxruntime
        URL "${_ort_url}"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_Populate(onnxruntime)

    set(
        ONNXRUNTIME_ROOT
        "${onnxruntime_SOURCE_DIR}"
        CACHE PATH "ONNX Runtime installation directory"
        FORCE
    )

endif()


# ============================================================
# FIND ONNX RUNTIME
# ============================================================

find_path(
    ONNXRUNTIME_INCLUDE_DIR
    onnxruntime_cxx_api.h
    PATHS "${ONNXRUNTIME_ROOT}/include"
    REQUIRED
    NO_DEFAULT_PATH
)

find_library(
    ONNXRUNTIME_LIBRARY
    onnxruntime
    PATHS "${ONNXRUNTIME_ROOT}/lib"
    REQUIRED
    NO_DEFAULT_PATH
)

add_library(
    onnxruntime::onnxruntime
    UNKNOWN
    IMPORTED
    GLOBAL
)

set_target_properties(
    onnxruntime::onnxruntime
    PROPERTIES
        IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
)


# ============================================================
# MODEL DIRECTORY
#
# This is ONLY a build/download location.
#
# IMPORTANT:
# We do NOT compile this path into the executable.
# The application finds the model relative to memora.exe
# at runtime.
# ============================================================

set(
    MEMORA_MODEL_BUILD_DIR
    "${CMAKE_BINARY_DIR}/model_download/all-MiniLM-L6-v2"
)

file(MAKE_DIRECTORY "${MEMORA_MODEL_BUILD_DIR}")


# ============================================================
# DOWNLOAD MINI-LM MODEL
# ============================================================

foreach(_model_file IN ITEMS
    onnx/model.onnx
    vocab.txt
    tokenizer_config.json
    special_tokens_map.json
    config.json
)

    get_filename_component(
        _model_name
        "${_model_file}"
        NAME
    )

    set(
        _model_output
        "${MEMORA_MODEL_BUILD_DIR}/${_model_name}"
    )

    set(_download_model TRUE)

    if(EXISTS "${_model_output}")

        file(
            SIZE
            "${_model_output}"
            _model_size
        )

        if(_model_size GREATER 0)
            set(_download_model FALSE)
        endif()

    endif()

    if(_download_model)

        message(
            STATUS
            "Downloading MiniLM model file: ${_model_file}"
        )

        file(
            DOWNLOAD
            "https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2/resolve/main/${_model_file}?download=true"
            "${_model_output}"
            SHOW_PROGRESS
            STATUS _model_status
            TLS_VERIFY ON
        )

        list(
            GET
            _model_status
            0
            _model_status_code
        )

        if(NOT _model_status_code EQUAL 0)

            message(
                FATAL_ERROR
                "Failed to download model asset ${_model_file}: ${_model_status}"
            )

        endif()

    endif()

endforeach()


# ============================================================
# SOURCE FILES
# ============================================================

file(
    GLOB_RECURSE
    MEMORA_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# The CLI contains main().
# Do NOT put CLI source files into the library.

list(
    FILTER
    MEMORA_SOURCES
    EXCLUDE
    REGEX
    ".*/src/cli/repl\\.cpp$"
)

list(
    FILTER
    MEMORA_SOURCES
    EXCLUDE
    REGEX
    ".*/src/cli/terminal\\.cpp$"
)


# ============================================================
# MEMORADB LIBRARY
# ============================================================

add_library(
    memora
    STATIC
    ${MEMORA_SOURCES}
)

target_include_directories(
    memora
    PUBLIC
        "${CMAKE_SOURCE_DIR}/src"
)

target_include_directories(
    memora
    PRIVATE
        "${ONNXRUNTIME_INCLUDE_DIR}"
)

target_link_libraries(
    memora
    PUBLIC
        onnxruntime::onnxruntime
)


# ============================================================
# WINDOWS COMPATIBILITY
# ============================================================

if(MINGW)

    target_compile_definitions(
        memora
        PRIVATE
            _stdcall=__stdcall
            ORT_DLL_IMPORT
    )

endif()


# ============================================================
# MEMORADB CLI
# ============================================================

add_executable(
    memora_cli

    src/cli/repl.cpp
    src/cli/terminal.cpp
)

target_link_libraries(
    memora_cli
    PRIVATE
        memora
)

set_target_properties(
    memora_cli
    PROPERTIES
        OUTPUT_NAME "memora"
)


# ============================================================
# RELEASE DIRECTORY
#
# Everything needed by the application goes here.
#
# release/
# ├── memora.exe
# ├── MinGW runtime DLLs
# ├── onnxruntime.dll
# ├── onnxruntime_providers_shared.dll
# └── models/
#     └── all-MiniLM-L6-v2/
# ============================================================

set(
    MEMORA_RELEASE_DIR
    "${CMAKE_BINARY_DIR}/release"
)

set_target_properties(
    memora_cli
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${MEMORA_RELEASE_DIR}"
)


# ============================================================
# COPY ONNX RUNTIME + MODEL + MINGW RUNTIME
# ============================================================

if(APPLE)
    set_target_properties(
        memora_cli
        PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@executable_path"
    )
elseif(UNIX AND NOT APPLE)
    set_target_properties(
        memora_cli
        PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            BUILD_RPATH "\$ORIGIN;\$ORIGIN/../lib;${ONNXRUNTIME_ROOT}/lib"
            INSTALL_RPATH "\$ORIGIN;\$ORIGIN/../lib;${ONNXRUNTIME_ROOT}/lib"
    )
    target_link_options(
        memora_cli
        PRIVATE
            -static-libgcc
            -static-libstdc++
    )
endif()

if(WIN32)

    get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)

    add_custom_command(
        TARGET memora_cli
        POST_BUILD

        # ----------------------------------------------------
        # Create model directory
        # ----------------------------------------------------

        COMMAND
            ${CMAKE_COMMAND}
            -E
            make_directory
            "${MEMORA_RELEASE_DIR}/models/all-MiniLM-L6-v2"


        # ----------------------------------------------------
        # ONNX Runtime
        # ----------------------------------------------------

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll"
            "${MEMORA_RELEASE_DIR}/onnxruntime.dll"

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_shared.dll"
            "${MEMORA_RELEASE_DIR}/onnxruntime_providers_shared.dll"


        # ----------------------------------------------------
        # MiniLM model
        # ----------------------------------------------------

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_directory
            "${MEMORA_MODEL_BUILD_DIR}"
            "${MEMORA_RELEASE_DIR}/models/all-MiniLM-L6-v2"


        # ----------------------------------------------------
        # MinGW runtime
        # ----------------------------------------------------

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_if_different
            "${_compiler_dir}/libstdc++-6.dll"
            "${MEMORA_RELEASE_DIR}/libstdc++-6.dll"

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_if_different
            "${_compiler_dir}/libgcc_s_seh-1.dll"
            "${MEMORA_RELEASE_DIR}/libgcc_s_seh-1.dll"

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_if_different
            "${_compiler_dir}/libwinpthread-1.dll"
            "${MEMORA_RELEASE_DIR}/libwinpthread-1.dll"


        COMMENT
            "Preparing self-contained MemoraDB release directory"
    )

else()

    add_custom_command(
        TARGET memora_cli
        POST_BUILD

        COMMAND
            ${CMAKE_COMMAND}
            -E
            make_directory
            "${MEMORA_RELEASE_DIR}/models/all-MiniLM-L6-v2"

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_directory
            "${MEMORA_MODEL_BUILD_DIR}"
            "${MEMORA_RELEASE_DIR}/models/all-MiniLM-L6-v2"

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_directory
            "${ONNXRUNTIME_ROOT}/lib"
            "${MEMORA_RELEASE_DIR}"

        COMMAND
            ${CMAKE_COMMAND}
            -E
            copy_directory
            "${ONNXRUNTIME_ROOT}/lib"
            "${CMAKE_BINARY_DIR}"

        COMMENT
            "Preparing MemoraDB release directory with full runtime libraries"
    )

endif()


# ============================================================
# BUILD INFORMATION
# ============================================================

message(STATUS "")

message(STATUS "==============================================")
message(STATUS "MemoraDB configuration complete")
message(STATUS "==============================================")

message(STATUS "Build directory  : ${CMAKE_BINARY_DIR}")
message(STATUS "Release directory: ${MEMORA_RELEASE_DIR}")
message(STATUS "Model build dir  : ${MEMORA_MODEL_BUILD_DIR}")

message(STATUS "==============================================")
message(STATUS "")

option(MEMORA_BUILD_TESTS "Build developer test suite" OFF)
if(MEMORA_BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/test/CMakeLists.txt")
    enable_testing()
    add_subdirectory(test)
endif()