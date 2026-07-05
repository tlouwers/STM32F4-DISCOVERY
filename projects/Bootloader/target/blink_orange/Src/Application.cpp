/**
 * \file    Application.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Application
 *
 * \brief   Composition root for the blink_orange sample application.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/blink_orange/Src
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Application.hpp"
#include "board/Board.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructs the application.
 * \param   bootloaderEntry   Shared factory-reset entry/trigger module.
 */
Application::Application(BootloaderEntry& bootloaderEntry) :
    mBootloaderEntry(bootloaderEntry)
{
}

/**
 * \brief   Brings up the clock, LED, and button.
 * \returns True (both drivers are unconditional GPIO setups; kept bool to
 *          match the project's fallible-Init() convention).
 */
bool Application::Init()
{
    Board::InitClock();
    Led::Init();
    Button::Init();
    return true;
}

/**
 * \brief   Polls the user button for a factory-reset request.
 * \details Blue user button (B1, PA0) arms a factory reset and jumps to the
 *          ST bootloader. Does not return when pressed.
 */
void Application::Process()
{
    if (Button::IsPressed())
    {
        mBootloaderEntry.TriggerFactoryReset();   // does not return
    }
}

/**
 * \brief   Handles a single command byte from the host protocol.
 * \param   command   Command byte; kFactoryResetCommand arms a factory reset.
 * \details Does not return when the factory-reset command is received.
 */
void Application::HandleCommand(uint8_t command)
{
    if (kFactoryResetCommand == command)
    {
        mBootloaderEntry.TriggerFactoryReset();   // does not return
    }
}

/**
 * \brief   Enters an infinite fast-blink error indication loop.
 */
void Application::Error()
{
    while (1)
    {
        Led::Toggle();
        HAL_Delay(250);
    }
}
