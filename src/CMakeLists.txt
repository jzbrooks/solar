project(compiler)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB lib_sources *.hpp *.cpp)
list(REMOVE_ITEM lib_sources ${PROJECT_SOURCE_DIR}/main.cpp)
message(${lib_sources})

# All sources that also need to be tested in unit tests go into a static library
add_library(compiler_lib STATIC ${lib_sources})

# Link against LLVM libraries
llvm_map_components_to_libnames(llvm_libs support core irreader)

# The main program
add_executable(compiler main.cpp)
target_link_libraries(compiler PRIVATE compiler_lib ${llvm_libs})
