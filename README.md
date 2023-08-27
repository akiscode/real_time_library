# Akiscode Real Time Library

A C and C++ library that implements common functionality with real-time constraints in mind.  

---

Real-time environments often can't dynamically allocate memory and require analysis of worst case execution time (WCET).  This results in code that can't use common abstractions like `std::vector` or `std::map`.  Additionally, some non-real-time embedded devices chose not to dynamically allocate for fear of fragmenting the memory space.  

To solve these problems, the Akiscode Real Time Library (RTL) was developed.  The RTL is split into two sub-libraries:

- `librtl` a pure C library that implements many low-level operations (like memory allocations) that aren't available on embedded systems.   
- `librtlcpp` a C++11 library that takes the building blocks found in `librtl` and creates abstractions like a vector or map, but does so in a way that is acceptable for real-time environments.

Both libraries can be used independently of each other.  More information can be found in the README files in each directory.

| librtl | librtlcpp|
| ---    | ---      |
| [Readme](librtl/README.md) | [Readme](librtlcpp/README.md)|

__Contact and Commercial Support:__ <dev@akiscode.com>

__Prerequisites__

- For `Both`:
    - CMake >= 3.11

- For `librtl`
    - A compiler that supports the C99 standard

- For `librtlcpp`
    - A compiler that supports the C++11 Standard

# Build

## Quickstart

```
git clone https://github.com/akiscode/real_time_library; cd real_time_library;
mkdir build && cd build;
cmake -DRTL_TARGET_WORD_SIZE_BITS=32 -DRTL_BUILD_ALL=ON .. && make;
```

## CMake Instructions

### Required

`RTL_TARGET_WORD_SIZE_BITS`

* Informs the library of the word size for the __TARGET__ platform.  E.g. if you were developing for a STM32 (32 bit word size) on a 64 bit machine, you would want to set this to 32.
* __Example Usage:__ `cmake -DRTL_TARGET_WORD_SIZE_BITS=16 ..`

### Optional

The RTL library has a number of optional parameters which are described below:

`RTL_BUILD_SHARED`

* When `ON`, builds `librtl` and `librtlcpp` as shared libraries instead of static ones.
* __Default Value:__ OFF
* __Example Usage:__ `cmake -DRTL_BUILD_SHARED=ON ..`

`RTL_BUILD_TESTS`

* Builds the unit tests for the library.
* __Default Value:__ OFF
* __Example Usage:__ `cmake -DRTL_BUILD_TESTS=ON ..`


`RTL_BUILD_BENCH`

* Builds the benchmark applications for the library.
* __Default Value:__ OFF
* __Example Usage:__ `cmake -DRTL_BUILD_BENCH=ON ..`

`RTL_BUILD_ALL`

* Convenience setting that will override `RTL_BUILD_TESTS` and `RTL_BUILD_BENCH` to both be on.
* __Default Value:__ OFF
* __Example Usage:__ `cmake -DRTL_BUILD_ALL=ON ..`

`RTL_BUILD_C_ONLY`

* Makes it so that only the C library `librtl` gets built and not `librtlcpp`
* __Default Value:__ OFF
* __Example Usage:__ `cmake -DRTL_BUILD_C_ONLY=ON ..`

## CMake External Project

This project can be quickly utilized with an external project add in CMake:

```
cmake_minimum_required(VERSION 3.11)

include(FetchContent)

# Set your options here
# e.g. 
#  set(RTL_BUILD_ALL ON)
#  set(RTL_BUILD_TESTS ON)
#  set(RTL_BUILD_BENCH ON)

# MUST set this variable
set(RTL_TARGET_WORD_SIZE_BITS 32)

FetchContent_Declare(
    akiscode-rtl
    GIT_REPOSITORY "https://github.com/akiscode/real_time_library.git"
    GIT_TAG main
    UPDATE_DISCONNECTED ON
)

FetchContent_GetProperties(akiscode-rtl)

if(NOT akiscode-rtl_POPULATED)
    FetchContent_Populate(akiscode-rtl)
    add_subdirectory(${akiscode-rtl_SOURCE_DIR} ${akiscode-rtl_BINARY_DIR})
endif()

add_executable(rtl_c_exe ... )
target_link_libraries(rtl_c_exe PRIVATE rtl)

add_executable(rtl_cpp_exe ... )
target_link_libraries(rtl_cpp_exe PRIVATE rtlcpp)
```
# License

Find all licensing information in the LICENSE file in this repository.

