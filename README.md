# STM32F407G-DISC1
Reusable C++ components for the STM32F407G-DISC1 kit.

## Contents

| Folder | Contents |
| ------ | -------- |
| Drivers/board | Helper class and configuration file to configure clock and pins of the board. |
| Drivers/components/HI-M1388AR | HI-M1388AR 8x8 LED matrix display class. Includes library with digits, letters and symbols to display. |
| Drivers/components/LIS3DSH | LIS3DSH accelerometer class with HW fifo support. |
| Drivers/drivers/ADC | ADC peripheral driver class. Simple and Interrupt based input capture for single channel. |
| Drivers/drivers/BasicTimer | BasicTimer peripheral driver class. Intended for use with DAC. |
| Drivers/drivers/DAC | DAC peripheral driver class. Simple and DMA based output for any waveform (uses BasicTimer). |
| Drivers/drivers/DMA | DMA utility class, intended as plug-in functionality for peripherals. |
| Drivers/drivers/GenericTimer | GenericTimer peripheral driver class. Provides period timer functionality. |
| Drivers/drivers/I2C | I2C peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| Drivers/drivers/Pin | Helper class intended as 'set & forget' for pin  configurations. State is preserved (partly) within the hardware. |
| Drivers/drivers/PWM | PWM peripheral driver class. Using Timer 2..4 as clock source. |
| Drivers/drivers/RTC | RTC peripheral driver class. Provides easier handling of Date and Time. |
| Drivers/drivers/SPI | SPI peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| Drivers/drivers/Usart | USART peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| Drivers/interfaces | Various interfaces for peripheral drivers. |
| Drivers/utility/Assert | Alternate 'assert' logic for embedded systems with more fine-grained control. |
| Drivers/utility/CpuWakeCounter | Helper class intended to put the CPU into a 'light' sleep mode and measure the wake percentage in one go. |
| Drivers/utility/HeapCheck | Low level functions to determine heap usage during run time. |
| Drivers/utility/StackPainting | Low level functions to determine stack usage during run time. |
| ExampleProject | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Showcases the LEDs and Accelerometer. |
| StandupCounter | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. Example StandupCounter with Buzzer and HI-M1388AR 8x8 LED matrix display. |
| tests | Folder with generic Fake and Mock content to assist unit testing. |
