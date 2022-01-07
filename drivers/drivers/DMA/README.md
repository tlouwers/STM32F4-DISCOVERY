
# DMA
DMA utility class.

## Description
Intended use is to provide a plug-n-play DMA object to 'Link' with a peripheral. This way a peripheral can be extended with DMA functionality in a more generic way without modifying the peripheral much. The complexity of DMA configuration is handled within this class.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- A configured peripheral, for example Usart

## Notes
The callbacks are called withing ISR context.
This class assumes the HAL has set NVIC_PRIORITYGROUP_4.
If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.

## Example
```cpp
// Declare the class (in Application.hpp for example):
DMA   mDMA_Usart2_Rx;
DMA   mDMA_Usart2_Tx;

// Construct the class(es), indicate the stream to use:
Application::Application() :
    mDMA_Usart2_Rx(DMA::Stream::Dma1_Stream5),
    mDMA_Usart2_Tx(DMA::Stream::Dma1_Stream6),
{}

// Configure the objects:
bool Application::Initialize()
{
    bool result = mDMA_Usart2_Tx.Configure(DMA::Channel::Channel4, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    assert(result);

    result = mDMA_Usart2_Rx.Configure(DMA::Channel::Channel4, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    assert(result);

    // Link the DMA objects with a previously configured peripheral, for example Usart:
    result = mDMA_Usart2_Tx.Link(mUsart2.GetPeripheralHandle(), mUsart2.GetDmaTxHandle());
    assert(result);

    result = mDMA_Usart2_Rx.Link(mUsart2.GetPeripheralHandle(), mUsart2.GetDmaRxHandle());
    assert(result);

    return result;
}

// To Write (DMA based):
uint8_t write_buffer[] = "test\r\n";
bool result = mUsart2.WriteDma(write_buffer, sizeof(write_buffer), [this]() { this->WriteDone(); } );
assert(result);

// To Read (DMA based):
uint8_t read_buffer[6] = {0};
result = mUsart2.ReadDma(read_buffer, sizeof(read_buffer), [this](uint16_t bytesReceived) { this->ReadDone(bytesReceived); });
assert(result);

// The ReadDone callback (as example):
void Application::ReadDone(uint16_t bytesReceived)
{
    if (bytesReceived > 0)
    {
        // Do stuff ...
    }
}

// The WriteDone callback (as example)
void Application::WriteDone()
{
    ;    // Flag, trigger next action, ...
}
```
