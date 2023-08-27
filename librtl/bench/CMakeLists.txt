project(RTL_BENCH CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cycle_counts cycle_counts.cpp )

target_link_libraries(cycle_counts rtl )
