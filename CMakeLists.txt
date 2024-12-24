cmake_minimum_required(VERSION 3.16)

# Project Name and Version
project(ztail VERSION 1.0 LANGUAGES CXX)

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
    -flto
    -pipe
)

if(ipo_supported)
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

# Find LibZip
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

if(LIBZIP_FOUND)
    message(STATUS "LibZip found: ${LIBZIP_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "LibZip not found!")
endif()

# Include Directories
include_directories(
    ${ZLIB_INCLUDE_DIRS}
    ${LIBZIP_INCLUDE_DIRS}
    src
)

# Source Files
set(SOURCES
    src/main.cpp
    src/circular_buffer.cpp
    src/cli.cpp
    src/compressor_zlib.cpp
    src/compressor_zip.cpp
    src/parser.cpp
)

# Executable
add_executable(ztail ${SOURCES})

# Link Libraries
target_link_libraries(ztail
    PRIVATE
        ${ZLIB_LIBRARIES}
        ${LIBZIP_LIBRARIES}
)
