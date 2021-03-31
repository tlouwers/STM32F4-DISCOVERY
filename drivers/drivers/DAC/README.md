
# DAC
DAC peripheral driver class.

## Description
Intended use is to provide an easier means to work with the DAC peripheral. This class assumes the pins to use for the DAC are already configured.
It has 2 modes implemented: either use only SetValue() to output a value or create a BasicTimer and DMA configuration and output a waveform (in a loop).
The latter requires a buffer filled with values of the precision configured (8 or 12 bit). The frequency of the BasicTimer is divided by the number of samples to get the frequency of the output rate (in Hz).

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- Pins already configured for DAC

## Notes
To provide more flexibility the BasicTimer is kept outside this class (allowing other timer triggers as well). DMA configuration is kept in line with peripherals as the USART and SPI.
 
## Example 1 (no DMA)
```cpp
// Declare the class (in Application.hpp for example):
Dac mDAC;

// Initialize the class:
bool Application::Initialize()
{
	// Initialize the DAC
	bool result = mDAC.Init();
	ASSERT(result);

	// Configure channel 1: using defaults
	result &= mDAC.ConfigureChannel(Dac::Channel::CHANNEL_1, Dac::ChannelConfig());
	ASSERT(result);

    // Other stuff...

	// Output a value: in DAC counts (in this case 12-bit)
	result &= mDAC.SetValue(Dac::Channel::CHANNEL_1, 4500);
	ASSERT(result);

    return result;
}
```

## Example 2 (with DMA, waveform)
(adding DMA and BasicTimer for completeness)
```cpp
// Declare the class (in Application.hpp for example):
BasicTimer mBasicTimer;
Dac        mDAC;
DMA        mDMA_DAC_Ch1;

// Construct the class, indicate the instance to use:
Application::Application() :
	mBasicTimer(BasicTimerInstance::TIMER_6),
	mDMA_DAC_Ch1(DMA::Stream::Dma1_Stream5),
{}

// Initialize the class:
bool Application::Initialize()
{
	// Initialize DMA for DAC channel 1
	bool result = mDMA_DAC_Ch1.Configure(DMA::Channel::Channel7, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Circular, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
	ASSERT(result);
	
	// Link DMA with the DAC
	result &= mDMA_DAC_Ch1.Link(mDAC.GetPeripheralHandle(), mDAC.GetDmaChannel1Handle());
	ASSERT(result);

	// Initialize the BasicTimer
	result &= mBasicTimer.Init(BasicTimer::Config(12, 10000)); // 10 kHz
	ASSERT(result);

	// Initialize the DAC
	result &= mDAC.Init();
	ASSERT(result);

	// Configure the waveform which is later output on the DAC
	result &= mDAC.ConfigureWaveform(Dac::Channel::CHANNEL_1, const_cast<uint16_t*>(sine_table), SINE_TABLE_LEN);
	ASSERT(result);

	// Configure channel 1: using BasicTimer6, configured before
	result &= mDAC.ConfigureChannel(Dac::Channel::CHANNEL_1, Dac::ChannelConfig(Dac::Precision::_12_BIT_R, Dac::Trigger::TIMER_6));
	ASSERT(result);

	// Start the 'heartbeat' timer for the DAC DMA
	result &= mBasicTimer.Start();
	ASSERT(result);

    // Other stuff...

	// Start the waveform output loop on DAC channel 1
	result &= mDAC.StartWaveform(Dac::Channel::CHANNEL_1);
	ASSERT(result);

    return result;
}
```