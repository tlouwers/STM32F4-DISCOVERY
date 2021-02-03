
#include "stm32f4xx_hal.h"
#include "board/Board.hpp"
#include "Application.hpp"
#include "utility/stack_painting.h"


int main()
{
    paint_stack();

    Board::InitPins();
    Board::InitClock();     // ignore result?


    HAL_Init();


    Application mApp;

    if (!mApp.Init())
    {
        mApp.Error();
    }

    while (1)
    {
        mApp.Process();
    }
}
