# STM32F407G-DISC1
Reusable C++ components for the STM32F407G-DISC1 kit.

## Contents

| Folder | Contents |
| ------ | -------- |
| Drivers/board | Helper class and configuration file to configure clock and pins of the board. |
| Drivers/components/LIS3DSH | LIS3DSH accelerometer class with HW fifo support. |
| Drivers/drivers/DMA | DMA utility class, intended as plug-in functionality for peripherals. |
| Drivers/drivers/I2C | I2C peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| Drivers/drivers/Pin | Helper class intended as 'set & forget' for pin  configurations. State is preserved (partly) within the hardware. |
| Drivers/drivers/PWM | PWM class. Using Timer 2 (hard-coded) as clock source - easily portable to other clock sources. |
| Drivers/drivers/SPI | SPI peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| Drivers/drivers/Usart | USART peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| Drivers/utility/CpuWakeCounter | Helper class intended to put the CPU into a 'light' sleep mode and measure the wake percentage in one go. |
| Drivers/utility/HeapCheck | Low level functions to determine heap usage during run time. |
| Drivers/utility/StackPainting | Low level functions to determine stack usage during run time. |
| ExampleProject | A Visual Studio Code, CMake, GCC, C++, Google Test, GCOV example project for STM32F407G-DISC1. |
