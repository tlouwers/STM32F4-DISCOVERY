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
        TestCrc.cpp
        TestRng.cpp
        TestDMA.cpp
        TestSPI.cpp
        TestI2C.cpp
        TestUSART.cpp
        # Mocks and Fakes
        Fake/drivers/Crc/Crc.cpp
        Fake/drivers/Pin/Pin.cpp
        Fake/utility/Assert/Assert.cpp
        Fake/stm32f4xx_hal.c
        Fake/stm32f4xx_hal_rng.cpp
        Fake/stm32f4xx_hal_dma.cpp
        Fake/stm32f4xx_hal_spi.cpp
        Fake/stm32f4xx_hal_i2c.cpp
        Fake/stm32f4xx_hal_usart.cpp
        # Test subjects
        ../drivers/components/HI-M1388AR/HI-M1388AR.cpp
        ../drivers/components/LIS3DSH/LIS3DSH.cpp
        ../drivers/drivers/Rng/Rng.cpp
        ../drivers/drivers/DMA/DMA.cpp
        ../drivers/drivers/SPI/SPI.cpp
        ../drivers/drivers/I2C/I2C.cpp
        ../drivers/drivers/USART/USART.cpp
        # Used sources (not part of unit tests)
        # Ex: ../target/Src/utility/crc32.cpp
)

# Include the real and Fake source directories.
# Fake comes first so its Pin/Assert/HAL stubs shadow the canonical headers for
# the same paths; canonical comes second for components/, interfaces/, etc.
target_include_directories(TestRunner PRIVATE
    ./Fake
    ../drivers
    .
)

# Link in Google Mock (which also includes Google Test)
target_link_libraries(TestRunner gmock_main)
