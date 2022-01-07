

# GenericTimer
GenericTimer peripheral driver class.

## Description
Intended use is to provide an easier means to work with Generic Timers. For now only PeriodElapsed callback is provided.
There are many other use cases and configuration possible for the generic timers, these are not supported by this code (yet).

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11

## Notes
The timer is assumed to be used as period elapsed trigger only.
Assumed is AHB1 / AHB2 is set to 8 MHz.

## Example
```cpp
// Declare the class (in Application.hpp for example):
GenericTimer mGenericTimer;

// Construct the class, indicate the instance to use:
Application::Application() :
    mGenericTimer(GenericTimerInstance::TIMER_2),
{}

// Initialize the class:
bool Application::Initialize()
{
    // Initialize the GenericTimer.
    bool result = mGenericTimer.Init(GenericTimer::Config(9, 2.1));    // 2.1 Hz
    ASSERT(result);

    // Start the period tick. Provide callback to call when period elapsed.
    result = mTim.Start( [this]() { this->TimerElapsed(); } );
    ASSERT(result);

    // Other stuff...

    return result;
}

// Callback called when timer elapses.
void Application::TimerElapsed()
{
    // ToDo: toggle led for instance.
}
```
