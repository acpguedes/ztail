cmake_minimum_required(VERSION 3.16)

# Project Name and Version
project(ztail VERSION 1.0 LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Position Independent Code for LTO
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable Link Time Optimization (LTO)
# CMake 3.9+ provides target_link_options for adding link flags
# Here, we use it to add LTO flags
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(WARNING "IPO / LTO not supported: <${ipo_error}>")
endif()

# Compiler Optimization Flags
# These flags apply to all targets
add_compile_options(
    -O3                    # High optimization level
    -march=native          # Optimize for the local machine architecture
    -flto                   # Enable Link Time Optimization
    -pipe                   # Use pipes rather than temporary files for communication between the various stages of compilation
)

# Linker Optimization Flags
# These flags are necessary for LTO
if(ipo_supported)
    add_link_options(
        -flto                # Enable Link Time Optimization
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

# For some systems, you might need to link additional libraries
# For example, on Linux, you might need to link with pthread or other system libraries
# Uncomment and modify the following lines if necessary
# find_package(Threads REQUIRED)
# target_link_libraries(ztail_modular PRIVATE Threads::Threads)

# Installation (Optional)
# You can specify installation rules if you plan to install the executable
# install(TARGETS ztail_modular DESTINATION bin)
