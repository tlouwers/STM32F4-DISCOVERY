
# DAC
BasicTimer peripheral driver class.

## Description
Intended use is to provide an easier means to work with Basic Timers. As these timers are intended to be used for the DAC only, no PeriodElapsed callback is provided.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11

## Notes
The timer is assumed to be used as trigger for the DMA for the DAC.
Assumed is AHB1 is set to 8 MHz.
 
## Example
```cpp
<TODO>
```
