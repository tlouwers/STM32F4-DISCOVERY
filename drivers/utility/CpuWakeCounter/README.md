
# CpuWakeCounter
Helper class intended to put the CPU into a 'light' sleep mode and measure the wake percentage in one go.

## Description
This code is intended to be used as tracing mechanism. It measures the wake percentage of the CPU and keeps track of the (main) loop count. At the end of the main loop call 'EnterSleepMode()' to sleep with either '__WFI()' or '__WFE()'. An interrupt is needed to wake the CPU again (read the ARM documentation for all possible wakeup sources).
By replacing the default enter/exit sleep mode call, some functionality is added to calculate the wake time of the CPU. Roughly each second an update of CpuStats is made available where details can be retrieved for the application to use. This code makes use of the DWT block as present on an ARM Cortex-M3, M4 or M7. This code was developed for an STM32F407G, but can be adapted for other microcontrollers as well.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- DWT block as present on an ARM Cortex-M3, M4 or M7

## Notes
This code replaces the WaitForInterrupt / WaitForEvent functionality and assumes the main process loop iterates at least once per second. Since it uses a sleep mode, the CPU must wake up by using an interrupt or event respectively.

## Example
```cpp
// Include the header
#include "CpuWakeCounter.hpp"

// Declare the object
CpuWakeCounter cpuWakeCounter;

// Initialize the class
bool Application::Initialize()
{
    bool result = cpuWakeCounter.Init();
    assert(result);

    // Other stuff...

    return result;
}

// In the main process loop
void Application::Process()
{
    // Do useful stuff
    delay_ms(100);                          // Mimics handling various items

    // Handle an update (if available)
    if (cpuWakeCounter.IsUpdated())         // Will update once per second
    {
        // Get the updated statistics
        CpuStats cpuStats = cpuWakeCounter.GetStatistics();

        // Handle the statistics, like log or assert if the wake percentage is above 80%
        if (cpuStats.wakePercentage > 80.0f)
        {
            assert(false);
        }
    }

    // At the end of the main process loop enter the desired sleep mode, per default the Systick is suspended in this method while sleeping (can be overruled).
    cpuWakeCounter.EnterSleepMode(SleepMode::WaitForInterrupt);
}
```
