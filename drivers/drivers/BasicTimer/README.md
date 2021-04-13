

# BasicTimer
BasicTimer peripheral driver class.

## Description
Intended use is to provide an easier means to work with Basic Timers. As these timers are intended to be used for the DAC only, no PeriodElapsed callback is provided.
Envisioned is to create and enable a 'heartbeat' tick of a certain frequency, which in turn is used by the DAC DMA to output values. Interrupts are used internally to signal such a 'tick'.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11

## Notes
The timer is assumed to be used as trigger for the DMA for the DAC.
Assumed is AHB1 is set to 8 MHz.
 
## Example
```cpp
// Declare the class (in Application.hpp for example):
BasicTimer  mBasicTimer;

// Note: left out the DAC and its DMA configuration here for clarity.

// Construct the class, indicate the instance to use:
Application::Application() :
	mBasicTimer(BasicTimerInstance::TIMER_7),
{}

// Initialize the class:
bool Application::Initialize()
{
	// Initialize the BasicTimer.
	bool result = mBasicTimer.Init(BasicTimer::Config(12, 10000)); // 10 kHz
	ASSERT(result);

    // DAC and its DMA configuration here.
    // DAC should configure a waveform to output (loop over)

	// Start the 'heartbeat' tick. Use the DAC to start/stop the waveform.
	result &= mBasicTimer.Start();
	ASSERT(result);

	// Start the DAC waveform.

	// Other stuff...

    return result;
}
```
