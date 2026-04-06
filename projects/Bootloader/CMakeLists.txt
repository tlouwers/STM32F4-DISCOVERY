cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

project(bootloader_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(3rd-party/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_subdirectory(tests)

# Note: bootloader_fw is built separately (cross-compiled via bootloader_fw/CMakeLists.txt)
# Note: host is built separately (host/CMakeLists.txt)
