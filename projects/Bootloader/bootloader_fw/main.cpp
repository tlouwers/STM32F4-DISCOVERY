/**
 * Bootloader entry point — stub (firmware not yet implemented).
 * Init → recovery check → boot app or enter update mode.
 */

#include "stm32f4xx_hal.h"

int main()
{
    HAL_Init();

    while (1)
    {
        // placeholder: bootloader logic goes here
    }

    return 0;
}
