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
        TestHI-M1388AR.cpp
        TestLIS3DSH.cpp
        # Mocks and Fakes
        Fake/drivers/Pin/Pin.cpp
        Fake/utility/Assert/Assert.cpp
        Fake/stm32f4xx_hal.c
        # Test subjects
        ../target/Src/components/HI-M1388AR/HI-M1388AR.cpp
        ../target/Src/components/LIS3DSH/LIS3DSH.cpp
        # Used sources (not part of unit tests)
        # Ex: ../target/Src/utility/crc32.cpp
)

# Include the real and Fake source directories
target_include_directories(TestRunner PRIVATE
    ../target/Src
    ./Fake
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunner gmock_main)
