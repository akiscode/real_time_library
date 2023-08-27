project(rtl C)
# C Standard/Version

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(RTL_SOURCE_FILES 
    rtl.c
    memory.c
    pounds.c
)

add_library(rtl ${RTL_LIBRARY_TYPE} ${RTL_SOURCE_FILES})

if (${RTL_LIBRARY_TYPE} STREQUAL "STATIC")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

target_compile_definitions(rtl
        PUBLIC
            RTL_TARGET_WORD_SIZE_BITS=${RTL_TARGET_WORD_SIZE_BITS}
        PRIVATE
            MAJOR_VERSION=${RTL_MAJOR_VERSION}
            MINOR_VERSION=${RTL_MINOR_VERSION}
            PATCH_VERSION=${RTL_PATCH_VERSION}
)

target_include_directories(rtl
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )


if (CMAKE_BUILD_TYPE STREQUAL "Debug" )
    # If we are building a release version of this library,
    # we don't want to cause newer compilers to inadvertently
    # fail the build

    target_compile_options(rtl PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
    )
endif()


INSTALL(TARGETS rtl
    EXPORT rtlLib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

INSTALL(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Make the project importable from build directory
EXPORT(TARGETS rtl FILE rtlConfig.cmake)

if(${RTL_BUILD_TESTS})

    include(${RTL_CMAKE_DIR}/googletest.cmake)
    include(CTest)
    include(GoogleTest)

    add_subdirectory(test)

endif()


if(${RTL_BUILD_BENCH})
    add_subdirectory(bench)
endif()
