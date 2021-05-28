
# Assert / EXPECT
Custom implementation of ASSERT() and EXPECT().

# Description
Helper functionality to replace/enhance the 'assert' logic for embedded systems with more fine-grained control.
Two macros are provided which can be used during development of an embedded system: EXPECT and ASSERT.
* An EXPECT() is used if you think the device can continue execution a little while longer. For instance: nothing will break inside the method where EXPECT() is placed. Checks around buffers if there is still room available could be of this type.
* An ASSERT() is used if at that point where the device cannot recover. For instance with a value that is absolutely not allowed and if used will stop the device from working.
Responses can be configured (and added if need be), like to log the incident, or to reset the board.

The header is implemented as classic '.h' file to allow it to be used in C code (although the system is expected to be a C++ project).

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
* A 'config.h' file in which the defines 'EXPECT_MODE' and 'ASSERT_MODE' are set.
 
# Notes
This code replaces the classic assert logic with more fine-grained logic, be sure to configure it properly.
 
# Example
```cpp
// Add the include file
#include "utility/Assert.h"

// Use where applicable. The example here is to claim memory for a buffer:
bool DebugConsole::Init()
{
    bool result = tx.Resize(DEBUG_CONSOLE_TX_SIZE);
    EXPECT(result);

    // More init stuff...
}

// Use where applicable. The example here is the Pin class to set an output pin to a level.
void Pin::Set(Level level)
{
    ASSERT(mDirection == Direction::OUTPUT);    // Cannot set level if pin is not configured as output

    // More set pin stuff...
}
```
