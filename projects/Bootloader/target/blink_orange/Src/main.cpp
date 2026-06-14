/**
 * Blink orange LED (LD3, PD13) — sample application for bootloader validation.
 */

#include "stm32f4xx_hal.h"
#include "drivers/Led.hpp"
#include "bootloader/BootloaderEntry.hpp"
#include "bootloader/Stm32BootloaderHal.hpp"
#include "Application.hpp"

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
