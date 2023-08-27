project(rtlcpp CXX)
# C++ Standard/Version

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RTLCPP_SOURCE_FILES
        rtlcpp.cpp
        allocator.cpp
        utility.cpp
        mutex.cpp
        vector.cpp
        map.cpp
        hash.cpp
        object_pool.cpp
        lru.cpp
        ring_buffer.cpp
        memory.cpp
        task.cpp
        )

add_library(rtlcpp ${RTL_LIBRARY_TYPE} ${RTLCPP_SOURCE_FILES})

if (${RTL_LIBRARY_TYPE} STREQUAL "STATIC")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

target_link_libraries(rtlcpp PUBLIC rtl)

target_compile_definitions(rtlcpp
        PUBLIC
            RTL_TARGET_WORD_SIZE_BITS=${RTL_TARGET_WORD_SIZE_BITS}
        PRIVATE
            MAJOR_VERSION=${RTL_MAJOR_VERSION}
            MINOR_VERSION=${RTL_MINOR_VERSION}
            PATCH_VERSION=${RTL_PATCH_VERSION}
)

target_include_directories(rtlcpp
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # If we are building a release version of this library,
    # we don't want to cause newer compilers to inadvertently
    # fail the build

    target_compile_options(rtlcpp PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
            )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rtlcpp PRIVATE -fno-exceptions -fno-rtti)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(rtlcpp PRIVATE /EHs-c- /GR-)
endif()


INSTALL(TARGETS rtlcpp
        EXPORT rtlcppLib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

INSTALL(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )

## Make the project importable from build directory
EXPORT(TARGETS rtlcpp FILE rtlcppConfig.cmake)

if (${RTL_BUILD_TESTS})

    include(${RTL_CMAKE_DIR}/googletest.cmake)
    include(CTest)
    include(GoogleTest)

    add_subdirectory(test)

endif ()


if (${RTL_BUILD_BENCH})
    add_subdirectory(bench)
endif ()


