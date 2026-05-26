/**
 * \file    stm32f4xx_hal_rng.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Compatibility shim so code that includes the vendor RNG sub-header
 *          name finds the same fake surface that lives in the umbrella header.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_RNG_H
#define __STM32F4xx_HAL_RNG_H


/* The RNG-fake surface (types, RNG/RCC pointers, HAL_RNG_* prototypes,
 * test observation hooks) is defined in the umbrella fake header so the
 * Rng driver and the TestRng case see exactly the same declarations. */
#include "stm32f4xx_hal.h"


#endif  // __STM32F4xx_HAL_RNG_H
