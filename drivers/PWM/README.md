
# Description
PWM class.

Intended use is to provide an easier means to work with PWM channels. For this driver it is hard-coded to timer 2, all 4 channels can be used. This class assumes the pins to use for the PWM channels are already configured.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
* Pins already configured for PWM channels

# Notes
The use of Timer 2 is hard-coded, as well as the assumption the APB1 timer clock source is set to 8 MHz. Future updates may lift these restrictions.
 
# Examples
```cpp
// Declare the class (in Application.hpp for example):
PWM   mPwm;

// Initialize the class to setup the PWM frequency:
bool result = mPwm.Init(PWM::Config(500));      // 500 Hz

// Configure a channel, here channel 1 using 50% duty cycle:
bool result = mPwm.ConfigureChannel(PWM::ChannelConfig(PWM::Channel::Channel_1, 50));

// To start the channel:
bool result = mPwm.Start(PWM::Channel::Channel_1);

// To stop a channel:
bool result = mPwm.Stop(PWM::Channel::Channel_1);
```
