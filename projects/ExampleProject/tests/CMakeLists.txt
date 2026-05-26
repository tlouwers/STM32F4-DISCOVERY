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
        TestAppLogic.cpp
        # Mocks and Fakes
        Fake/drivers/Pin/Pin.cpp
        Fake/utility/Assert/Assert.cpp
        Fake/stm32f4xx_hal.c
        # Used sources (not part of unit tests)
        ../target/Src/AppLogic.cpp
)

# Include the real and Fake source directories.
# Fake comes first so its Pin/Assert/HAL stubs shadow the canonical headers for
# the same paths; canonical comes second for components/, interfaces/, etc.
target_include_directories(TestRunner PRIVATE
    ./Fake
    ../../../drivers       # canonical: components/, interfaces/, utility/ (non-faked)
    ../../../tests/Mock    # shared GMock objects (Mock_LIS3DSH, Mock_Watchdog, Mock_Pin)
    ../target/Src          # project-specific: AppLogic.hpp, board/BoardConfig.hpp, config.h
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunner gmock_main)
