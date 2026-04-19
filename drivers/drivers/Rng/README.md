
# Rng
Hardware random number generator class.

## Description
Hardware random number generator class. Uses analog noise to generate a 32-bit random number.
According to the datasheet: RNG passed FIPS PUB 140-2 (2001 October 10) tests with a success ratio of 99%.

## Requirements
- ST Microelectronics STM32F407G-DISC1
- C++11

## Notes
If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.

## Example
```cpp
// Declare the class (in Application.hpp for example):
Rng mRng;


// Initialize the class:
bool Application::Initialize()
{
    // Initialize the CRC module.
    bool result = mRng.Init();
    ASSERT(result);

    // Other stuff...

    return result;
}

// Generate a random number:
uint32_t Application::Example()
{
    return = mRng.GetRandom();          // Blocking call, 40 clock cycles @ 48 MHz (PLL clock)
}
```
