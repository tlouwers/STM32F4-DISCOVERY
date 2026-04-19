
# ADC
ADC peripheral driver class.

## Description
Intended use is to provide an easier means to work with the ADC peripheral. This class assumes the pins to use for the ADC are already configured.
It has 2 modes implemented: either use only GetValue() as blocking method to get a value from the ADC input, or use a variant which uses interrupts (non-blocking).

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- Pins already configured for ADC

## Notes
All data is right aligned. No DMA methods are implemented as the ADC class is envisioned to be used to retrieve a value sporadically.
Also no multichannel is implemented.

## Example 1 (Polling/Blocking)
```cpp
// Declare the class (in Application.hpp for example):
Adc mADC;

// Initialize the class:
bool Application::Initialize()
{
    // Initialize the ADC
    bool result = mADC.Init(Adc::Config(13, Adc::Channel::CHANNEL_11, Adc::Resolution::_12_BIT));
    ASSERT(result);

    // Other stuff...

    // Sample a value: in ADC counts (in this case 12-bit)
    uint16_t sample = 0;
    result &= mADC.GetValue(sample);
    ASSERT(result);

    return result;
}
```

## Example 2 (Interrupt)
```cpp
// Declare the class (in Application.hpp for example):
Adc mADC;

// Initialize the class:
bool Application::Initialize()
{
    // Initialize the ADC
    bool result = mADC.Init(Adc::Config(13, Adc::Channel::CHANNEL_11, Adc::Resolution::_12_BIT));
    ASSERT(result);

    // Other stuff...

    // Sample a value: in ADC counts (in this case 12-bit). This starts a single
    // conversion, when completed the handler is called with the result.
    result &= mADC.GetValueInterrupt([this](uint16_t  value) { this->AdcDataReceived(value); });
    ASSERT(result);

    return result;
}

// Handler for the ADC value captured callback
void Application::AdcDataReceived(uint16_t value)
{
    // Do something with 'value'
}
```
