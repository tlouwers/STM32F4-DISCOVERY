# STM32F407G-DISC1
Reusable C++ components for the STM32F407G-DISC1 kit.

## Contents

| Folder | Contents |
| ------ | -------- |
| drivers/board | Helper class and configuration file to configure clock and pins of the board. |
| drivers/components/HI-M1388AR | HI-M1388AR 8x8 LED matrix display class. Includes library with digits, letters and symbols to display. |
| drivers/components/LIS3DSH | LIS3DSH accelerometer class with HW fifo support. |
| drivers/drivers/Adc | Adc peripheral driver class. Simple and Interrupt based input capture for single channel. |
| drivers/drivers/BasicTimer | BasicTimer peripheral driver class. Intended for use with DAC. |
| drivers/drivers/Crc | Crc peripheral driver class. Uses hardware CRC module of the STM32F4. |
| drivers/drivers/Dac | Dac peripheral driver class. Simple and DMA based output for any waveform (uses BasicTimer). |
| drivers/drivers/DMA | DMA utility class, intended as plug-in functionality for peripherals. |
| drivers/drivers/GenericTimer | GenericTimer peripheral driver class. Provides period timer functionality. |
| drivers/drivers/I2C | I2C peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| drivers/drivers/Pin | Helper class intended as 'set & forget' for pin configurations. State is preserved (partly) within the hardware. |
| drivers/drivers/PWM | PWM peripheral driver class. Using Timer 2..4 as clock source. |
| drivers/drivers/Rng | Hardware random number generator. Uses PLL (40 clock cycles) and analog noise to generate true 32-bit random number. |
| drivers/drivers/Rtc | Rtc peripheral driver class. Provides easier handling of Date and Time. |
| drivers/drivers/SPI | SPI peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| drivers/drivers/TimerIRQ | TimerIRQ infrastructure: single owner of all STM32F407 timer interrupt vectors, fans shared IRQ lines to per-timer dispatch slots. |
| drivers/drivers/USART | USART peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| drivers/drivers/Watchdog | Watchdog (IWDG) peripheral driver class. |
| drivers/interfaces | Various interfaces for peripheral drivers. |
| drivers/utility/Assert | Alternate 'assert' logic for embedded systems with more fine-grained control. |
| drivers/utility/CpuWakeCounter | Helper class intended to put the CPU into a 'light' sleep mode and measure the wake percentage in one go. |
| drivers/utility/HeapCheck | Low level functions to determine heap usage during run time. |
| drivers/utility/StackPainting | Low level functions to determine stack usage during run time. |
| freertos | Shared FreeRTOS kernel source (LTS release), used by FreeRTOS-based projects. Per-project FreeRTOSConfig.h lives in each project's target/Src/. |
| hal | Shared STM32F4 HAL and CMSIS headers (v1.28.3), referenced by all project targets. |
| projects/ExampleProject | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Showcases the LEDs and Accelerometer. |
| projects/FreeRTOSProject | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Basic example to showcase use of FreeRTOS. |
| projects/StandupCounter | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Example StandupCounter with Buzzer and HI-M1388AR 8x8 LED matrix display. |
| projects/TiltExample | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Example with FreeRTOS using the accelerometer and HI-M1388AR 8x8 LED matrix display to show if device is being tilted. |
| projects/UnitTestExample | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Example how to work with unit tests. |
| tests | Folder with generic Fake and Mock content to assist unit testing. |
