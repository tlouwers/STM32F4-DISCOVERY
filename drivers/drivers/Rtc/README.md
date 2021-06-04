
# Rtc
Rtc peripheral driver class.

## Description
Intended use is to provide an easier means to work with the Real Time Clock, providing a settable/gettable Date and Time sturcture.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11

## Notes
The clock source of the RTC must be configured correctly, so take care.
Per default the LSI is selected, as the STM32F4-DISCOVERY board has no external 32 kHz crystal mounted.
This clock (and the HSE for that matter) will not be as accurate as the LSE (with crystal).

## Example
```cpp
// Declare the class (in Application.hpp for example):
Rtc mRtc;

// Initialize the class:
bool Application::Initialize()
{
    // Initialize the RTC - be sure to select the proper clock source.
    bool result = mRtc.Init(Rtc::Config(Rtc::ClockSource::LSI));
    ASSERT(result);

    // At this point the RTC runs, but with default date and time.

    // Configure the proper date and time (retreived via external UART maybe?)
    DateTime dateTime = { 2021, 6, 4, 10, 4, 25 };
    bool result = mRtc.SetDateTime(dateTime);
    ASSERT(result);

    // Other stuff...

    return result;
}

// Retrieve the date and time (for instance when logging something)
void Application::Log(const uint8_t* message, uint16_t length)
{
    DateTime dateTime = {};
    bool result = mRtc.GetDateTime(dateTime);
    ASSERT(result);

    if (result)
    {
        // ToDo: actual logging...
    }
}
```
