# STM32F407G-DISC1
Reusable C++ components for the STM32F407G-DISC1 kit.

## Contents

| Folder | Contents |
| ------ | -------- |
| board | Helper class and configuration file to configure clock and pins of the board. |
| components/LIS3DSH | LIS3DSH accelerometer class with HW fifo support. |
| drivers/DMA | DMA utility class, intended as plug-in functionality for peripherals. |
| drivers/Pin | Helper class intended as 'set & forget' for pin  configurations. State is preserved (partly) within the hardware. |
| drivers/PWM | PWM class. Using Timer 2 (hard-coded) as clock source - easily portable to other clock sources. |
| drivers/SPI | SPI peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| drivers/Usart | USART peripheral driver class. Has blocking and asynchronous (DMA and interrupt based) methods. |
| utility/CpuWakeCounter | Helper class intended to put the CPU into a 'light' sleep mode and measure the wake percentage in one go. |
