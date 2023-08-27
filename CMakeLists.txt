cmake_policy(SET CMP0048 NEW)
project(AkiscodeRTL)
cmake_minimum_required(VERSION 3.11)


OPTION(RTL_BUILD_ALL "Build unit tests and benchmark tests" OFF)
OPTION(RTL_BUILD_TESTS "Build Unit Tests" OFF)
OPTION(RTL_BUILD_BENCH "Build Benchmark Tests" OFF)
OPTION(RTL_BUILD_SHARED "Build shared libraries when on, otherwise build static when off" OFF)
OPTION(RTL_BUILD_C_ONLY "Only build the rtl C library, not rtlcpp" OFF)


# -------------------------------------------

if (NOT DEFINED RTL_TARGET_WORD_SIZE_BITS OR NOT RTL_TARGET_WORD_SIZE_BITS)
    MESSAGE(FATAL_ERROR "RTL_TARGET_WORD_SIZE_BITS is not defined or is empty.  Please set it.\n --> Example: cmake -DRTL_TARGET_WORD_SIZE_BITS=32 ...")
endif ()

MESSAGE("RTL: Setting Target Word Size Bits to: ${RTL_TARGET_WORD_SIZE_BITS}")

if (${RTL_BUILD_ALL})
    set(RTL_BUILD_TESTS ON)
    set(RTL_BUILD_BENCH ON)
endif ()


if (CMAKE_SYSTEM_PROCESSOR STREQUAL "")
    set(CMAKE_SYSTEM_PROCESSOR "unknown")
endif ()


if (${RTL_BUILD_SHARED})
    set(RTL_LIBRARY_TYPE "SHARED")
else ()
    set(RTL_LIBRARY_TYPE "STATIC")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PACKAGE_TYPE_STR "debug")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PACKAGE_TYPE_STR "prod")
else ()
    set(PACKAGE_TYPE_STR "dev")
endif ()

# -------------------------------------------

# Project Info

set(RTL_MAJOR_VERSION 1)
set(RTL_MINOR_VERSION 1)
set(RTL_PATCH_VERSION 0)
set(RTL_VERSION ${RTL_MAJOR_VERSION}.${RTL_MINOR_VERSION}.${RTL_PATCH_VERSION})


# -------------------------------------------

MESSAGE("RTL: CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
MESSAGE("RTL Options set")
MESSAGE("-> RTL_BUILD_ALL: " ${RTL_BUILD_ALL})

if (${RTL_BUILD_ALL})
    MESSAGE("-> RTL_BUILD_TESTS (RTL_BUILD_ALL Override): " ${RTL_BUILD_TESTS})
    MESSAGE("-> RTL_BUILD_BENCH (RTL_BUILD_ALL Override): " ${RTL_BUILD_BENCH})
else ()
    MESSAGE("-> RTL_BUILD_TESTS: " ${RTL_BUILD_TESTS})
    MESSAGE("-> RTL_BUILD_BENCH: " ${RTL_BUILD_BENCH})
endif ()
MESSAGE("-> RTL_BUILD_SHARED: " ${RTL_BUILD_SHARED})
MESSAGE("-> RTL_TARGET_WORD_SIZE_BITS: " ${RTL_TARGET_WORD_SIZE_BITS})
MESSAGE("-> RTL_BUILD_C_ONLY: " ${RTL_BUILD_C_ONLY})


# -------------------------------------------

include(GNUInstallDirs)
include(ExternalProject)

if (${RTL_BUILD_TESTS})
    include(CTest)
endif ()

set(RTL_CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)

add_subdirectory(librtl)

if (NOT ${RTL_BUILD_C_ONLY})
    add_subdirectory(librtlcpp)
endif ()


# Package Info
set(CPACK_PACKAGE_VERSION_MAJOR ${RTL_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${RTL_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${RTL_PATCH_VERSION})

set(CPACK_PACKAGE_FILE_NAME "rtl-${RTL_TARGET_WORD_SIZE_BITS}bit-${RTL_MAJOR_VERSION}.${RTL_MINOR_VERSION}.${RTL_PATCH_VERSION}-${CMAKE_SYSTEM_PROCESSOR}-${PACKAGE_TYPE_STR}")

# Include Cpack after setting variables for them to take effect
include(CPack)
