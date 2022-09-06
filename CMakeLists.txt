cmake_minimum_required(VERSION 3.23)
project(allocator)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_executable(mini_alloc_test test/mini_alloc_test.cc)
add_executable(cross_alloc_test test/cross_alloc_test.cc)
