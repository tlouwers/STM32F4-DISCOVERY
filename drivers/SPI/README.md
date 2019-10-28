# Description
SPI peripheral driver class.

Intended use is to provide an easier means to work with the SPI peripheral. This class assumes the pins to use for the SPI are already configured.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
* Pins already configured for SPI

# Notes
Master only.
The ChipSelect is to be toggled manually (outside the class).
The callbacks are called within ISR context.
This class assumes the HAL has set NVIC_PRIORITYGROUP_4.
 
# Examples
```cpp
// Declare the class (in Application.hpp for example):
SPI mSPI;

// Construct the class, indicate the instance to use:
Application::Application() :
    mSPI(SPIInstance::SPI_1)
{}

// Initialize the class:
bool Application::Initialize()
{
    bool result = mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));
    assert(result);

    // Other stuff...

    return result;
}

// To Write (interrupt based):
uint8_t write_buffer[] = "test\r\n";
bool result = mSPI.WriteInterrupt(write_buffer, sizeof(write_buffer), [this]() { this->WriteDone(); } );
assert(result);

// To Read (interrupt based):
uint8_t read_buffer[6] = {0};
result = mSPI.ReadInterrupt(read_buffer, sizeof(read_buffer), [this]() { this->ReadDone(); });
assert(result);

// The ReadDone callback (as example):
void Application::ReadDone()
{
    // Do stuff...
}
```
