
# Watchdog
Watchdog (IWDG) peripheral driver class.

## Description
Intended use is to provide an easy means to use the Watchdog (IWDG). This class assumes the required LSI clock is already configured.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- LSI already configured.

## Notes
When debugging the watchdog timer is freezed. Can be enabled/disabled in code.

## Example
```cpp
// Declare the class (in Application.hpp for example):
Watchdog mWatchdog;

// Initialize the class:
bool Application::Initialize()
{
    // This also starts the watchdog
    bool result = mWatchdog.Init(Watchdog::Config(Watchdog::Timeout::_4_S));     // 4 seconds
    ASSERT(result);

    // Other stuff...

    return result;
}

// Refresh the watchdog (within the configured timeout):
void Application::Process()
{
    // Other stuff

    // Refresh watchdog every once in a while - before the 4 second timeout
    mWatchdog.Refresh();
}
```
