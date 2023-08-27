project(RTL_TEST CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


enable_testing()

add_executable(rtl_test
    main.cpp
)

target_compile_options(rtl_test PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(rtl_test rtl gtest_main)

# ---

get_target_property(RTL_INCLUDES rtl INCLUDE_DIRECTORIES)
get_target_property(RTL_SRC rtl SOURCE_DIR)


add_executable(memory_test
    memory_test.cpp
    )

target_include_directories(memory_test PUBLIC
    ${RTL_SRC}
    ${RTL_SRC}/include
    )

target_compile_definitions(memory_test PUBLIC
        RTL_TARGET_WORD_SIZE_BITS=${RTL_TARGET_WORD_SIZE_BITS}
        PUBLIC IMPL_RTL_MEMORY_TEST)


target_compile_options(memory_test PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)


target_link_libraries(memory_test gtest_main)

# ---

gtest_discover_tests(rtl_test memory_test)
add_test(rtl_test rtl_test)


