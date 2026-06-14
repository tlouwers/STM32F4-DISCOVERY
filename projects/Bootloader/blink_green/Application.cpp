#include "Application.hpp"
#include "board/Board.hpp"
#include "stm32f4xx_hal.h"

Application::Application(BootloaderEntry& bootloaderEntry) :
    mBootloaderEntry(bootloaderEntry)
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

void Application::HandleCommand(uint8_t command)
{
    if (kFactoryResetCommand == command)
    {
        mBootloaderEntry.TriggerFactoryReset();   // does not return
    }
}

void Application::Error()
{
    while (1)
    {
        Led::Toggle();
        HAL_Delay(250);
    }
}
