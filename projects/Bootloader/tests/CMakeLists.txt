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
        TestBootloaderEntry.cpp
        # Fakes
        Fake/stm32f4xx_hal.c
        # Test subjects (shared firmware modules; Stm32BootloaderHal.cpp is
        # firmware-only and substituted here by MockBootloaderHal)
        ../target/shared/bootloader/BootloaderEntry.cpp
        # Ex: ../target/shared/<module>/<Module>.cpp
)

# Include real, shared, Mock, and Fake source directories
target_include_directories(TestRunner PRIVATE
    ../target/shared
    ./Mock
    ./Fake
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunner gmock_main)
