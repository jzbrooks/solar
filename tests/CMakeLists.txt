project(solar_tests)

find_package(LLVM REQUIRED CONFIG)

set(CMAKE_GENERATOR_PLATFORM x64)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB lib_sources *.hpp *.cpp)
list(REMOVE_ITEM lib_sources ${PROJECT_SOURCE_DIR}/main.cpp)
message(${lib_sources})

llvm_map_components_to_libnames(llvm_libraries ${LLVM_TARGETS_TO_BUILD} core irreader)

file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

# The test program
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE ${llvm_libraries} solar_lib)

add_test(NAME all COMMAND tests)
