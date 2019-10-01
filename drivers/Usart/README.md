# Description
USART peripheral driver class.

Intended use is to provide an easier means to work with the USART peripheral. This class assumes the pins to use for the USART are already configured.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
* Pins already configured for USART

# Notes
When using the IDLE line feature during an Rx transmission, there will be an Rx and an IDLE line interrupt. Only 1 callback to Rx is called with the correct number of bytes.
The callbacks are called withing ISR context.
This class assumes the HAL has set NVIC_PRIORITYGROUP_4.
 
# Examples
```cpp
// Declare the class (in Application.hpp for example):
Usart mUsart;

// Construct the class, indicate the instance to use:
Application::Application() :
    mUsart(UsartInstance::USART_2)
{}

// Initialize the class:
bool Application::Initialize()
{
    bool result = mUsart.Init(Usart::Config(10, false, Usart::Baudrate::_9600));
    assert(result);
	
	// Other stuff...
	
	return result;
}

// To Write (interrupt based):
uint8_t write_buffer[] = "test\r\n";
bool result = mUsart.WriteInterrupt(write_buffer, sizeof(write_buffer), [this]() { this->WriteDone(); } );
assert(result);

// To Read (interrupt based):
uint8_t read_buffer[6] = {0};
result = mUsart.ReadInterrupt(read_buffer, sizeof(read_buffer), [this](uint16_t bytesReceived) { this->ReadDone(bytesReceived); });
assert(result);

// The ReadDone callback (as example):
void Application::ReadDone(uint16_t bytesReceived)
{
    if (bytesReceived > 0)
    {
        // Do stuff ...
    }
}
```
