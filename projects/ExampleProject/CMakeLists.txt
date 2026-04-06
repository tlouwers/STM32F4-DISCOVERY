# Define the minimum CMake version to use
cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

# Define the toplevel project
project(Project1_STM32F4 LANGUAGES C CXX)

# Define the C++ standard to use
set(CMAKE_CXX_STANDARD 14)              # Be able to compile C++14.
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # That C++14 is required.
set(CMAKE_CXX_EXTENSIONS OFF)           # Do not enable compiler-specific extensions -- this means that GCC will be given -std=c++14 rather than -std=gnu++14.

# Include the Google test framework
enable_testing()
add_subdirectory(3rd-party/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

# Include the unit tests
add_subdirectory(tests)

# Note: the folder 'target' is left out - it is build seperately (cross-compiled)
