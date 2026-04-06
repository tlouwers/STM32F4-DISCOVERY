#include "Application.hpp"
#include "board/Board.hpp"
#include "stm32f4xx_hal.h"

Application::Application()
{
}

bool Application::Init()
{
    Board::InitClock();
    Led::Init();
    return true;
}

void Application::Process()
{
}

void Application::Error()
{
    while (1)
    {
        Led::Toggle();
        HAL_Delay(250);
    }
}
