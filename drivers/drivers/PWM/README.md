
# PWM
PWM peripheral driver class.

## Description
Intended use is to provide an easier means to work with PWM channels. For this driver it is hard-coded to Timer2..5, all 4 channels can be used. This class assumes the pins to use for the PWM channels are already configured.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- Pins already configured for PWM channels

## Notes
Inspiration from: <https://stm32f4-discovery.net/2014/05/stm32f4-stm32f429-discovery-pwm-tutorial/>
The use of Timer2..5 is hard-coded, as well as the assumption the APB1 timer clock source is set to 8 MHz. Future updates may lift these restrictions.
 If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.
 
## Example
```cpp
// Declare the class (in Application.hpp for example):
PWM mPwm;

// Construct the class, indicate the instance to use:
Application::Application() :
    mPWM(PwmTimerInstance::TIMER_2)
{}

// Initialize the class to setup the PWM frequency:
bool result = mPwm.Init(PWM::Config(500));      // 500 Hz

// Configure a channel, here channel 1 using 50% duty cycle:
bool result = mPWM.ConfigureChannel(PWM::ChannelConfig(PWM::Channel::Channel_1, 50, PWM::Polarity::High));

// To start the channel:
bool result = mPwm.Start(PWM::Channel::Channel_1);

// To stop a channel:
bool result = mPwm.Stop(PWM::Channel::Channel_1);
```
