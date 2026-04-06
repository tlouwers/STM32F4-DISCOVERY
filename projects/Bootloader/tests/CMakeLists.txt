# The executable, starting point for the unit tests
add_executable(TestRunner TestRunner.cpp)

set(CMAKE_GENERATOR "Ninja")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")

# List of the sources and unit-test files
target_sources(TestRunner
    PRIVATE
        # Unit test files
        TestRunner.cpp
        TestBootloader.cpp
        # Fakes
        Fake/stm32f4xx_hal.c
        # Test subjects (add bootloader_fw modules here as they are extracted from main)
        # Ex: ../bootloader_fw/BootDecision.cpp
        # Ex: ../bootloader_fw/FlashDriver.cpp
        # Ex: ../bootloader_fw/Protocol.cpp
)

# Include real and Fake source directories
target_include_directories(TestRunner PRIVATE
    ../bootloader_fw
    ./Fake
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunner gmock_main)
