

# Description
Helper class which can be used as replacement for the WaitForInterrupt / WaitForEvent functionality. This adds  added CPU statistics like a percentage the CPU is awake and a (main) loop count. This can be useful for gathering statistics of the load of the CPU.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
 
# Notes
This code replaces the WaitForInterrupt / WaitForEvent functionality and assumes the main process loop iterates at least once per second. Since it uses a sleep mode, the CPU must wake up by using an interrupt or event respectively.
 
# Example
```cpp
// Include the header
#include "CpuWakeCounter.hpp"

// Declare the object
CpuWakeCounter cpuWakeCounter;

// In the main process loop
void Application::Process()
{
    // Do useful stuff
    delay_ms(100);                          // Mimicks handling various items

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
