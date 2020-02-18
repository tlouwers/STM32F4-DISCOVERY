
# Description
Helper class

Intended use it to have a single grouping of functionality which sets the clock and pins of the board into a defined state, and later into a defined sleep state (for low power behavior).

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed

# Notes
This is not an extensive list, more pins and functionality is added when I encounter it when hacking away with this board.
 
# Examples
```cpp
// Declare the class (in main.cpp, the top level class/file):
#include "board/Board.hpp"

int main(void)
{
    // Initialize the clock first:
    if (!Board::InitClock()) { assert(false); }

    // Initialize the pins:
    Board::InitPins();

    // As example, construct, initialize and run the remainder of the application:
    Application mApp;

    if (!mApp.Init())
    {
        mApp.Error();
    }

    while (true)
    {
        mApp.Process();
    }
}
```
