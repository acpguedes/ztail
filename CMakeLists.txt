cmake_minimum_required(VERSION 3.16)

# Project Name and Version
project(ztail VERSION 1.0 LANGUAGES CXX)

# Export version information to the compiler so it can be queried at runtime
add_compile_definitions(ZTAIL_VERSION="${PROJECT_VERSION}")

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Position Independent Code for LTO
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable Link Time Optimization (LTO)
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(WARNING "IPO / LTO not supported: <${ipo_error}>")
endif()

# Compiler Optimization Flags
add_compile_options(
    -O3
    -march=native
    -pipe
)

if(ipo_supported)
    add_compile_options(-flto)
    add_link_options(
        -flto
    )
endif()

# Find ZLIB
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    message(STATUS "ZLIB found: ${ZLIB_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "ZLIB not found!")
endif()

# Find BZip2
find_package(BZip2 REQUIRED)
if(BZIP2_FOUND)
    message(STATUS "BZip2 found: ${BZIP2_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "BZip2 not found!")
endif()

# Find LibZip
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)
pkg_check_modules(LIBLZMA REQUIRED liblzma)
pkg_check_modules(LIBZSTD REQUIRED libzstd)

if(LIBZIP_FOUND)
    message(STATUS "LibZip found: ${LIBZIP_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "LibZip not found!")
endif()
if(LIBLZMA_FOUND)
    message(STATUS "LibLZMA found: ${LIBLZMA_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "LibLZMA not found!")
endif()
if(LIBZSTD_FOUND)
    message(STATUS "LibZSTD found: ${LIBZSTD_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "LibZSTD not found!")
endif()

# Include Directories
include_directories(
    ${ZLIB_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIRS}
    ${BZIP2_INCLUDE_DIRS}
    ${LIBLZMA_INCLUDE_DIRS}
    ${LIBZSTD_INCLUDE_DIRS}
    src
)

# Enable testing infrastructure
include(CTest)

# Source Files
set(ZTAIL_LIB_SOURCES
    src/circular_buffer.cpp
    src/cli.cpp
    src/compressor_zlib.cpp
    src/compressor_bzip2.cpp
    src/compressor_xz.cpp
    src/compressor_zip.cpp
    src/compressor_zstd.cpp
    src/compression_type.cpp
    src/tail_plain.cpp
    src/parser.cpp
)

# Build library with core functionality
add_library(ztail_lib ${ZTAIL_LIB_SOURCES})
target_link_libraries(ztail_lib
    PUBLIC
        ${ZLIB_LIBRARIES}
        ${BZIP2_LIBRARIES}
        ${LIBLZMA_LIBRARIES}
        ${LIBZIP_LIBRARIES}
        ${LIBZSTD_LIBRARIES}
)
target_include_directories(ztail_lib PUBLIC src)

# Executable
add_executable(ztail src/main.cpp)
target_link_libraries(ztail PRIVATE ztail_lib)

# ------------------------------------------------------------------------------
# Tests
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(ztail_tests
        tests/test_circular_buffer.cpp
        tests/test_compressor_zlib.cpp
        tests/test_compressor_bzip2.cpp
        tests/test_compressor_xz.cpp
        tests/test_compressor_zip.cpp
        tests/test_compressor_zstd.cpp
        tests/test_parser.cpp
        tests/test_detection.cpp
        src/main.cpp
    )
    target_link_libraries(ztail_tests PRIVATE gtest_main ztail_lib)
    target_compile_definitions(ztail_tests PRIVATE ZTAIL_NO_MAIN)

    include(GoogleTest)
    gtest_discover_tests(ztail_tests)
endif()

install(TARGETS ztail DESTINATION bin)
