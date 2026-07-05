/**
 * \file    main.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Blink green LED (LD4, PD12) — sample application for bootloader validation.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/blink_green/Src
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "stm32f4xx_hal.h"
#include "drivers/Led.hpp"
#include "bootloader/BootloaderEntry.hpp"
#include "bootloader/Stm32BootloaderHal.hpp"
#include "Application.hpp"


/************************************************************************/
/* Main application entry point                                         */
/************************************************************************/
/**
 * \brief       Main application entry point.
 * \returns     Never returns.
 */
int main()
{
    // Factory-reset entry check: must run before any peripheral init. If the
    // application armed a factory reset before resetting, this hands control to
    // the ST system memory bootloader and does not return.
    Stm32BootloaderHal bootloaderHal;
    BootloaderEntry    bootloaderEntry(bootloaderHal);
    bootloaderEntry.CheckAndEnterBootloader();

    HAL_Init();

    Application app(bootloaderEntry);

    if (!app.Init())
    {
        app.Error();
    }

    while (1)
    {
        app.Process();
        Led::Toggle();
        HAL_Delay(500);
    }

    return 0;
}
