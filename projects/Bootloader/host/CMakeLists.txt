# Define the minimum CMake version to use
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# Define the toplevel project
project(host_tests LANGUAGES C CXX)

# Define the C++ standard to use
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include the Google test framework
enable_testing()
add_subdirectory(3rd-party/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Include the unit tests
add_subdirectory(tests)

# Note: the host src/ files are compiled directly into the test runner
# via tests/CMakeLists.txt (same pattern as ExampleProject).
