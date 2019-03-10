
# Description
Helper class intended as 'set & forget' for pin  configurations. State is preserved (partly) within the hardware.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed

# Notes
All pin interrupts share the same interrupt priority, which can be set via INTERRUPT_PRIORITY in the cpp file.
This class assumes the HAL has set NVIC_PRIORITYGROUP_4.
Per default, priority 5 is used.

## Warning
It is quite easy to bypass this class and set pins outside of it. For convenience, choose either style, but don't mix them up to prevent issues.
 
# Examples
```cpp
// Constructors
Pin a1(id, Level::LOW);                     // Construction - output
Pin a2(id, PullUpDown::DOWN);               // Construction - input, pull-down

// As output
a1.Set(Level::HIGH);                        // Set output high
a1.Toggle();                                // High to low, low to high
Level lvl1 = a1.Get();                      // Get the actual pin level
a1.Configure(PullUpDown::HIGHZ);            // Make pin input, floating

// As Input:
Level lvl2 = a2.Get();                      // Get the actual pin level
a2.Interrupt(Trigger::RISING, callback);    // Configure interrupt, attach callback, default active
a2.InterruptDisable();                      // Disable the callback (ignores the interrupt)
a2.InterruptEnable();                       // Enables the callback
a2.InterruptRemove();                       // Removed the interrupt, detaches the callback

// Allowed (moves):
Pin a4 = std::move(a1);                     // Move assignment
Pin a5(std::move(a2));                      // Move constructor

// Not allowed (copies):
Pin a7 = a1;                                // Copy assignment
Pin a8(a2);                                 // Copy constructor
Pin();                                      // No empty constructor
```
