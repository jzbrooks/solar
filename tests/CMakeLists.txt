project(solar_tests)

file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

# The test program
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE solar_lib)

add_test(NAME all COMMAND tests)
