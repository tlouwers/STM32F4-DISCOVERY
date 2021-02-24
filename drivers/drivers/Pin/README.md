
# Pin
Helper class intended as 'set & forget' for pin  configurations. State is preserved (partly) within the hardware.

## Description
Intended use is to have a method at board startup which sets each pin to a defined state. This is done by constructing a Pin object, and let it go out of scope.
Later in the application, for the few pins where needed, pass along the PinIdPort struct to the class where a pin object is needed.
Then during the initialisation of that class (not construction) create and fill the Pin object with desired values. At this point the interrupts can be configured as well.

The intent is to have a "BoardConfiguration.hpp" header file, in which all (used) pins of the device are listed, for example:
```cpp
#include "drivers/Pin/Pin.hpp"     // Used for the PinIdPort struct

// Button
constexpr PinIdPort PIN_BUTTON     = { GPIO_PIN_0,  GPIOA };
// Led
constexpr PinIdPort PIN_LED_GREEN  = { GPIO_PIN_12, GPIOD };
```

This can then be used as more human readable named ID of a pin. In a "Board.cpp" file a class with only static methods can be used to set all pins in their 'initial' state, meaning the pins are prepared for use by low level peripheral drivers, like USART, and leds can be off. Another method would be a 'sleep' method, which puts all pins into an appropriate lowest power consuming state - this is used for when the CPU is put into a sleep mode.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11

## Notes
All pin interrupts share the same interrupt priority, which can be set via INTERRUPT_PRIORITY in the cpp file.
This class assumes the HAL has set NVIC_PRIORITYGROUP_4.
Per default, priority 5 is used.
If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.

## Warning
It is quite easy to bypass this class and set pins outside of it. For convenience, choose either style, but don't mix them up to prevent issues.
 
## Example
```cpp
// Declaration (in header file, say in Application.hpp):
Pin a1;                                    // Note: requires construction during construction of owning class
Pin a2;

// Construct during construction of Application.cpp:
Application::Application() :
    mButton(PIN_BUTTON, Level::LOW),        // Externally pulled down
    mLedGreen(PIN_LED_GREEN, Level::LOW)    // Off
{
    // Other things ...
}

// As output:
a1.Set(Level::HIGH);                        // Set output high
a1.Toggle();                                // High to low, low to high
Level lvl1 = a1.Get();                      // Get the actual pin level
a1.Configure(PullUpDown::HIGHZ);            // Make pin input, floating

// As input:
Level lvl2 = a2.Get();                      // Get the actual pin level
a2.Interrupt(Trigger::RISING, callback);    // Configure interrupt, attach callback, default active
a2.InterruptDisable();                      // Disable the callback (ignores the interrupt)
a2.InterruptEnable();                       // Enables the callback
a2.InterruptRemove();                       // Removed the interrupt, detaches the callback

// As alternate (check the options of the CPU first!):
Pin(PIN_USART2_RTS, Alternate::AF7);        // See the HAL 'GPIO_Alternate_function_selection' for options
Pin(PIN_USART2_TX,  Alternate::AF7, PullUpDown::UP);

// Allowed (moves):
Pin a4 = std::move(a1);                     // Move assignment
Pin a5(std::move(a2));                      // Move constructor

// Not allowed (copies):
Pin a7 = a1;                                // Copy assignment
Pin a8(a2);                                 // Copy constructor
Pin();                                      // Empty constructor
```
