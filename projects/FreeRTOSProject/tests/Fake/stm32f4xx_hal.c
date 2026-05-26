/**
 * \file    stm32f4xx_hal.c
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Fake STM32F4 HAL implementation for FreeRTOSProject native unit
 *          tests. Provides no-op bodies for HAL entry points the drivers use.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/FreeRTOSProject/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "stm32f4xx_hal.h"

// Fake implementation done in 'C' file, to prevent multiple definitions.


// __NOP() formally part of CMSIS, only available for ARM.
void __NOP(void) { ; }

void HAL_Delay(uint32_t Delay) { ; }
