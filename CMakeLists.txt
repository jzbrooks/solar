cmake_minimum_required (VERSION 3.16)
project(solar)

set(CMAKE_CXX_STANDARD 17)
set(LLVM_DIR "${PROJECT_SOURCE_DIR}/llvm/lib/cmake/llvm/")

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
