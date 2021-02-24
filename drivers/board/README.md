
# Board
Board configuration helper files.

## Description
Intended use is to have a single grouping of functionality which sets the clock and pins of the board into a defined state, and later into a defined sleep state (for low power behavior).
- Board.hpp/cpp: intended to have a single grouping of functionality which sets the clock and pins of the board into a defined state, and later into a defined sleep state (for low power behavior).
-  BoardConfig.hpp: intended to be a human readable list of all pins of the system.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11 is assumed

## Notes
This is not an extensive list, more pins and functionality is added when I encounter it when hacking away with this board.
If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.
 
## Example
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
