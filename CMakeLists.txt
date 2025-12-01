cmake_minimum_required(VERSION 3.21)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(libcuebin
    VERSION 0.1.0
    DESCRIPTION "CUE sheet parser library for PSX emulation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBCUEBIN_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(src)

if(LIBCUEBIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
