project(RTLCPP_BENCH CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(unordered_map_bench unordered_map_bench.cpp )

target_link_libraries(unordered_map_bench rtlcpp )

add_executable(ring_buffer_bench ring_buffer_bench.cpp)

target_link_libraries(ring_buffer_bench pthread rtlcpp )
