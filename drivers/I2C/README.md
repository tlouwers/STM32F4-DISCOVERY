# Description
I2C peripheral driver class.

Intended use is to provide an easier means to work with the I2C peripheral. This class assumes the pins to use for the I2C are already configured.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
* Pins already configured for I2C

# Notes
Master only. Only 7-bit address.
The callbacks are called within ISR context.
This class assumes the HAL has set NVIC_PRIORITYGROUP_4.
 
# Examples
```cpp
// Declare the class (in Application.hpp for example):
I2C mI2C;

// Construct the class, indicate the instance to use:
Application::Application() :
    mI2C(I2CInstance::I2C_1)
{}

// Initialize the class:
bool Application::Initialize()
{
    bool result = mI2C.Init(I2C::Config(12, I2C::BusSpeed::NORMAL));
    assert(result);

    // Other stuff...

    return result;
}

// Example method to read the CHIP_ID from the CS43L22 audio amplifier chip
void Application::TestReadIdCS43L22()
{
    const uint8_t DEVICE_ADDRESS = 0x94;
    const uint8_t CHIP_ID        = 0x01;

    uint8_t reg = CHIP_ID;
    uint8_t dest = 0;

    bool result = false;
    // First write to the slave to indicate which register we want to read from
    if (mI2C.WriteBlocking(DEVICE_ADDRESS, &reg, 1))
    {
        // Then read 1 byte from the slave (the register indicated before)
        result = mI2C.ReadBlocking(DEVICE_ADDRESS, dest, 1);
        if (result)
        {
            // Do stuff with the received variable
        }
    }
```
