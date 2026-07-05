cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

project(bootloader_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(3rd-party/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_subdirectory(tests)

# Note: device firmware (target/) is built separately, cross-compiled for ARM
#       via target/CMakeLists.txt + target/arm-none-eabi-gcc.cmake.
# Note: the host tool (host/) is a separate .NET solution (host/BootloaderTool.sln).
