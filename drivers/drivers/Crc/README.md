
# Crc
Crc peripheral driver class.

## Description
Crc peripheral driver, uses the STM32F4 hardware CRC module to calculate a CRC32 with fixed polynomal (0x4C11DB7).

## Requirements
- ST Microelectronics STM32F407G-DISC1
- C++11

## Notes
If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.

## Example
```cpp
// Declare the class (in Application.hpp for example):
Crc mCrc;


// Initialize the class:
bool Application::Initialize()
{
    // Initialize the CRC module.
    bool result = mCrc.Init();
    ASSERT(result);

    // Other stuff...

    return result;
}

// Perform a CRC calculation:
uint32_t Application::Example()
{
    uint32_t dummy_data = { 0x1234, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789 };

    uint32_t crc_ result = mCrc.Calculate( dummy_data, (sizeof(dummy_data)/sizeof(dummy_data[0])) );

    return result;
}
```
