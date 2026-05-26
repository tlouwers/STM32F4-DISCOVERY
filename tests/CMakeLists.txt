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
        TestDac.cpp
        TestAdc.cpp
        TestGenericTimer.cpp
        TestBasicTimer.cpp
        TestPWM.cpp
        TestTimerIRQ.cpp
        TestRtc.cpp
        TestWatchdog.cpp
        TestPin.cpp
        TestHeapCheck.cpp
        TestCpuWakeCounter.cpp
        # Mocks and Fakes
        Fake/drivers/Crc/Crc.cpp
        Fake/utility/Assert/Assert.cpp
        Fake/stm32f4xx_hal.c
        Fake/stm32f4xx_hal_rng.cpp
        Fake/stm32f4xx_hal_dma.cpp
        Fake/stm32f4xx_hal_spi.cpp
        Fake/stm32f4xx_hal_i2c.cpp
        Fake/stm32f4xx_hal_usart.cpp
        Fake/stm32f4xx_hal_dac.cpp
        Fake/stm32f4xx_hal_adc.cpp
        Fake/stm32f4xx_hal_tim.cpp
        Fake/stm32f4xx_hal_rtc.cpp
        Fake/stm32f4xx_hal_iwdg.cpp
        Fake/stm32f4xx_hal_gpio.cpp
        # Test subjects
        ../drivers/components/HI-M1388AR/HI-M1388AR.cpp
        ../drivers/components/LIS3DSH/LIS3DSH.cpp
        ../drivers/drivers/Rng/Rng.cpp
        ../drivers/drivers/DMA/DMA.cpp
        ../drivers/drivers/SPI/SPI.cpp
        ../drivers/drivers/I2C/I2C.cpp
        ../drivers/drivers/USART/USART.cpp
        ../drivers/drivers/Dac/Dac.cpp
        ../drivers/drivers/Adc/Adc.cpp
        ../drivers/drivers/GenericTimer/GenericTimer.cpp
        ../drivers/drivers/BasicTimer/BasicTimer.cpp
        ../drivers/drivers/PWM/PWM.cpp
        ../drivers/drivers/TimerIRQ/TimerIRQ.cpp
        ../drivers/drivers/Rtc/Rtc.cpp
        ../drivers/drivers/Watchdog/Watchdog.cpp
        ../drivers/drivers/Pin/Pin.cpp
        ../drivers/utility/HeapCheck/heap_check.c
        ../drivers/utility/CpuWakeCounter/CpuWakeCounter.cpp
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
