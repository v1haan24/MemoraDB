cmake_minimum_required(VERSION 3.20)

project(VectorEmbedding)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the active virtual environment
set(Python3_FIND_VIRTUALENV FIRST)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
# Get the site-packages directory of the active Python
# Directory containing the active Python executable
get_filename_component(PYTHON_BIN_DIR "${Python3_EXECUTABLE}" DIRECTORY)

# Go from .venv/Scripts -> .venv
get_filename_component(PYTHON_VENV_DIR "${PYTHON_BIN_DIR}" DIRECTORY)

# Build the site-packages path
set(PYTHON_SITE_PACKAGES
    "${PYTHON_VENV_DIR}/Lib/site-packages"
)
file(TO_CMAKE_PATH "${PYTHON_SITE_PACKAGES}" PYTHON_SITE_PACKAGES)

message(STATUS "Python site-packages : ${PYTHON_SITE_PACKAGES}")


message(STATUS "Python executable : ${Python3_EXECUTABLE}")
message(STATUS "Python include    : ${Python3_INCLUDE_DIRS}")
message(STATUS "Python libraries  : ${Python3_LIBRARIES}")

# Location of pybind11 installed in the venv
set(pybind11_DIR
    "${PYTHON_SITE_PACKAGES}/pybind11/share/cmake/pybind11"
)

find_package(pybind11 CONFIG REQUIRED)

add_executable(VectorEmbedding
    cpp/main.cpp
    cpp/EmbeddingEngine.cpp
    cpp/Benchmark.cpp
)

target_include_directories(VectorEmbedding PRIVATE cpp)

target_link_libraries(VectorEmbedding
    PRIVATE
    pybind11::embed
    Python3::Python
)

target_compile_definitions(VectorEmbedding PRIVATE
    PYTHON_SITE_PACKAGES="${PYTHON_SITE_PACKAGES}"
)
