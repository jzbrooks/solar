cmake_minimum_required (VERSION 3.12)
project(compiler)

set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
