

# STM32F407G-DISC1
Reusable C++ components for the STM32F407G-DISC1 kit.

## Contents

| Folder | Contents |
| ------ | -------- |
| drivers/Pin | Helper class intended as 'set & forget' for pin  configurations. State is preserved (partly) within the hardware. |
| drivers/Usart | USART peripheral driver class. Has blocking and asynchronous (interrupt based) methods. |
| utility/CpuWakeCounter | Helper class intended to put the CPU into a 'light' sleep mode and measure the wake percentage in one go. |
