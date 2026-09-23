cmake_minimum_required(VERSION 3.20)

project(MemoraDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

include(FetchContent)

set(ONNXRUNTIME_VERSION "1.22.0" CACHE STRING "ONNX Runtime release to use")
if(NOT ONNXRUNTIME_ROOT)
    if(WIN32)
        if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|amd64|x86_64")
            message(FATAL_ERROR "Automatic ONNX Runtime download currently supports Windows x64 only")
        endif()
        set(_ort_archive "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip")
        set(_ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_ort_archive}")
    elseif(APPLE)
        
        set(_ort_archive "onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}.tgz")
        set(_ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_ort_archive}")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
        set(_ort_archive "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz")
        set(_ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_ort_archive}")
    else()
        message(FATAL_ERROR "Unsupported platform for automatic ONNX Runtime download. Set ONNXRUNTIME_ROOT to a compatible local runtime.")
    endif()
    FetchContent_Declare(onnxruntime URL "${_ort_url}" DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_Populate(onnxruntime)
    set(ONNXRUNTIME_ROOT "${onnxruntime_SOURCE_DIR}" CACHE PATH "ONNX Runtime installation directory" FORCE)

    file(GLOB _ort_pdb_files "${ONNXRUNTIME_ROOT}/lib/*.pdb")
    if(_ort_pdb_files)
        file(REMOVE ${_ort_pdb_files})
    endif()

    file(GLOB _ort_dsym_dirs "${ONNXRUNTIME_ROOT}/lib/*.dSYM")
    foreach(_ort_dsym_dir IN LISTS _ort_dsym_dirs)
        file(REMOVE_RECURSE "${_ort_dsym_dir}")
    endforeach()

    file(REMOVE_RECURSE "${ONNXRUNTIME_ROOT}/lib/cmake")
    file(REMOVE_RECURSE "${ONNXRUNTIME_ROOT}/lib/pkgconfig")
endif()

find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
    PATHS "${ONNXRUNTIME_ROOT}/include" REQUIRED NO_DEFAULT_PATH)
find_library(ONNXRUNTIME_LIBRARY onnxruntime
    PATHS "${ONNXRUNTIME_ROOT}/lib" REQUIRED NO_DEFAULT_PATH)

add_library(onnxruntime::onnxruntime UNKNOWN IMPORTED GLOBAL)
set_target_properties(onnxruntime::onnxruntime PROPERTIES
    IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}")

set(MEMORA_MODEL_DIR "${CMAKE_BINARY_DIR}/models/all-MiniLM-L6-v2")
file(MAKE_DIRECTORY "${MEMORA_MODEL_DIR}")
foreach(_model_file IN ITEMS onnx/model.onnx vocab.txt)
    get_filename_component(_model_name "${_model_file}" NAME)
    set(_model_output "${MEMORA_MODEL_DIR}/${_model_name}")
    set(_download_model TRUE)
    if(EXISTS "${_model_output}")
        file(SIZE "${_model_output}" _model_size)
        if(_model_size GREATER 0)
            set(_download_model FALSE)
        endif()
    endif()
    if(_download_model)
        file(DOWNLOAD
            "https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2/resolve/main/${_model_file}?download=true"
            "${_model_output}"
            SHOW_PROGRESS
            STATUS _model_status
            TLS_VERIFY ON)
        list(GET _model_status 0 _model_status_code)
        if(NOT _model_status_code EQUAL 0)
            message(FATAL_ERROR "Failed to download model asset ${_model_file}: ${_model_status}")
        endif()
    endif()
endforeach()

file(GLOB_RECURSE MEMORA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

list(FILTER MEMORA_SOURCES EXCLUDE REGEX
    ".*/src/cli/repl\\.cpp$"
)
list(FILTER MEMORA_SOURCES EXCLUDE REGEX
    ".*/src/cli/terminal\\.cpp$"
)
list(FILTER MEMORA_SOURCES EXCLUDE REGEX
    ".*/src/cli/help\\.cpp$"
)
add_library(memora STATIC ${MEMORA_SOURCES})
target_include_directories(memora
    PUBLIC
        "${CMAKE_SOURCE_DIR}/src"
)
target_link_libraries(memora PUBLIC onnxruntime::onnxruntime)
target_include_directories(memora PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
target_compile_definitions(memora PRIVATE MEMORA_MODEL_DIR="${MEMORA_MODEL_DIR}")
if(MINGW)
    target_compile_definitions(memora PRIVATE _stdcall=__stdcall ORT_DLL_IMPORT)
endif()

add_executable(memora_cli
    src/cli/repl.cpp
    src/cli/terminal.cpp
    src/cli/help.cpp
)
target_link_libraries(memora_cli PRIVATE memora)
target_compile_definitions(memora_cli PRIVATE MEMORA_MODEL_DIR="${MEMORA_MODEL_DIR}")
set_target_properties(memora_cli PROPERTIES
    OUTPUT_NAME memora
)


if(WIN32)
    add_custom_command(TARGET memora_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll" "$<TARGET_FILE_DIR:memora_cli>/onnxruntime.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/onnxruntime_providers_shared.dll" "$<TARGET_FILE_DIR:memora_cli>/onnxruntime_providers_shared.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MEMORA_MODEL_DIR}" "$<TARGET_FILE_DIR:memora_cli>/models/all-MiniLM-L6-v2")
    if(MINGW)
        get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        add_custom_command(TARGET memora_cli POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_compiler_dir}/libstdc++-6.dll" "$<TARGET_FILE_DIR:memora_cli>/libstdc++-6.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_compiler_dir}/libgcc_s_seh-1.dll" "$<TARGET_FILE_DIR:memora_cli>/libgcc_s_seh-1.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_compiler_dir}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:memora_cli>/libwinpthread-1.dll")
    endif()
elseif(APPLE)
    set_target_properties(memora_cli PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path")
    add_custom_command(TARGET memora_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_LIBRARY}" "$<TARGET_FILE_DIR:memora_cli>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MEMORA_MODEL_DIR}" "$<TARGET_FILE_DIR:memora_cli>/models/all-MiniLM-L6-v2")
else()
    set_target_properties(memora_cli PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN;$ORIGIN/../lib")
    target_link_options(memora_cli PRIVATE
        -static-libgcc
        -static-libstdc++)
    add_custom_command(TARGET memora_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_LIBRARY}" "$<TARGET_FILE_DIR:memora_cli>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${MEMORA_MODEL_DIR}" "$<TARGET_FILE_DIR:memora_cli>/models/all-MiniLM-L6-v2")
endif()

option(MEMORA_BUILD_TESTS "Build developer test suite" OFF)
if(MEMORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()