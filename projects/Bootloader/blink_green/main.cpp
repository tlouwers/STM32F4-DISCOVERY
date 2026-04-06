/**
 * Blink green LED (LD4, PD12) — sample application for bootloader validation.
 */

#include "stm32f4xx_hal.h"
#include "drivers/Led.hpp"
#include "Application.hpp"

int main()
{
    HAL_Init();

    Application app;

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
