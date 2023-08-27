project(RTL_CPP_TEST CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


enable_testing()

add_executable(rtl_cpp_test
        container.cpp
        hash.cpp
        object_pool.cpp
        map.cpp
        utility.cpp
        lru.cpp
        memory.cpp
        ring_buffer.cpp
        task.cpp
        )

target_compile_options(rtl_cpp_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
        )

target_link_libraries(rtl_cpp_test rtlcpp gtest_main)

# ---

gtest_discover_tests(rtl_cpp_test)
add_test(rtl_cpp_test rtl_cpp_test)

