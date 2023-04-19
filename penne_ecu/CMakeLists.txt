cmake_minimum_required(VERSION 3.21)
project(penne_ecu C)
set(CMAKE_C_STANDARD 17)
include(CTest)

# Output directory structure configuration.
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(TEST_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}/test")

# Project-wide include directory configuration.
include_directories("${PROJECT_SOURCE_DIR}/include")

# Project source lives in src/.
add_subdirectory(src)

# CTest sets BUILD_TESTING option to ON by default.
# Test-related configuration goes here.
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()



