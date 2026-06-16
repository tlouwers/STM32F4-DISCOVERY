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
    Button::Init();
    return true;
}

void Application::Process()
{
    // Blue user button (B1, PA0) arms a factory reset and jumps to the ST
    // bootloader. Does not return when pressed.
    if (Button::IsPressed())
    {
        mBootloaderEntry.TriggerFactoryReset();   // does not return
    }
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
