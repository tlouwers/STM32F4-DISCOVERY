/**
 * \file    stm32f4xx_hal_gpio.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Test-only observation/control hooks for the native fake STM32F4 HAL
 *          GPIO surface. The GPIO types, macros and HAL_GPIO_* prototypes live
 *          in the umbrella stm32f4xx_hal.h (the Pin driver includes only the
 *          umbrella); this header exposes the fake's control knobs so a test can
 *          drive HAL_GPIO_ReadPin and observe the Init/Write/Toggle calls. Only
 *          TestPin includes this -- the driver never does.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_GPIO_H
#define __STM32F4xx_HAL_GPIO_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake GPIO state: read state to RESET, counters + last-applied to 0. */
void FakeGPIO_Reset(void);

void          FakeGPIO_SetReadPinState(GPIO_PinState state);  ///< Value returned by HAL_GPIO_ReadPin (drives Pin::Get)
GPIO_PinState FakeGPIO_LastWriteState(void);                  ///< Last state passed to HAL_GPIO_WritePin (Pin::Set)
int           FakeGPIO_InitCallCount(void);                   ///< HAL_GPIO_Init invocation count
int           FakeGPIO_WriteCallCount(void);                  ///< HAL_GPIO_WritePin invocation count
int           FakeGPIO_ToggleCallCount(void);                 ///< HAL_GPIO_TogglePin invocation count
uint32_t      FakeGPIO_LastInitMode(void);                    ///< Mode of the last HAL_GPIO_Init config
uint32_t      FakeGPIO_LastInitPull(void);                    ///< Pull of the last HAL_GPIO_Init config


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_GPIO_H
