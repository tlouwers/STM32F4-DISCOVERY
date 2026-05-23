/**
 * \file    stm32f4xx_hal_tim.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL TIM (time-base) surface for the
 *          unit-test build. Declares the counter-mode / clock-division /
 *          auto-reload-preload macros, the RCC TIM clock gating macros and the
 *          HAL_TIM_Base_* prototypes the real timer drivers compile against.
 *          The TIM types/instances themselves live in the umbrella
 *          stm32f4xx_hal.h.
 *
 * \details This is the SHARED timer surface: GenericTimer (L26) created it,
 *          and BasicTimer (L27), PWM (L28) and TimerIRQ (L29) extend the same
 *          two files rather than duplicating a parallel surface. Every
 *          HAL_TIM_* entry point returns HAL_OK by default; the
 *          FakeTIM_Set*Result hooks drive each failure branch independently
 *          (Init, DeInit). The Start/Stop bodies are no-op HAL_OK (the drivers
 *          ignore their result). HAL_TIM_IRQHandler counts its invocations and,
 *          having reached the driver via the registered TimerIRQ slot, drives
 *          the driver's HAL_TIM_PeriodElapsedCallback override so the
 *          period-elapsed dispatch path can be exercised natively.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_TIM_H
#define __STM32F4xx_HAL_TIM_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Config-value macros (mirror the real HAL)                            */
/************************************************************************/
#define TIM_COUNTERMODE_UP                  ((uint32_t)0x00000000U)
#define TIM_COUNTERMODE_DOWN                ((uint32_t)0x00000010U)

#define TIM_CLOCKDIVISION_DIV1              ((uint32_t)0x00000000U)
#define TIM_CLOCKDIVISION_DIV2              ((uint32_t)0x00000100U)
#define TIM_CLOCKDIVISION_DIV4              ((uint32_t)0x00000200U)

#define TIM_AUTORELOAD_PRELOAD_DISABLE      ((uint32_t)0x00000000U)
#define TIM_AUTORELOAD_PRELOAD_ENABLE       ((uint32_t)0x00000080U)


/************************************************************************/
/* RCC TIM clock gating -- pretend the clock is already enabled.        */
/************************************************************************/
#define __HAL_RCC_TIM1_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM2_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM3_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM4_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM5_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM6_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM7_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM8_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM9_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_TIM10_CLK_ENABLE()   do { } while(0)
#define __HAL_RCC_TIM11_CLK_ENABLE()   do { } while(0)
#define __HAL_RCC_TIM12_CLK_ENABLE()   do { } while(0)
#define __HAL_RCC_TIM13_CLK_ENABLE()   do { } while(0)
#define __HAL_RCC_TIM14_CLK_ENABLE()   do { } while(0)

#define __HAL_RCC_TIM1_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM2_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM3_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM4_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM5_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM6_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM7_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM8_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM9_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_TIM10_CLK_DISABLE()  do { } while(0)
#define __HAL_RCC_TIM11_CLK_DISABLE()  do { } while(0)
#define __HAL_RCC_TIM12_CLK_DISABLE()  do { } while(0)
#define __HAL_RCC_TIM13_CLK_DISABLE()  do { } while(0)
#define __HAL_RCC_TIM14_CLK_DISABLE()  do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_tim.cpp)          */
/************************************************************************/
HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef* htim);
HAL_StatusTypeDef HAL_TIM_Base_DeInit(TIM_HandleTypeDef* htim);
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef* htim);
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef* htim);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef* htim);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef* htim);
void              HAL_TIM_IRQHandler(TIM_HandleTypeDef* htim);

/* Defined by the driver (GenericTimer.cpp); declared here so the definition
   links with C linkage and the fake HAL_TIM_IRQHandler can dispatch into it. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake TIM state: every result back to HAL_OK, counters to 0. */
void FakeTIM_Reset(void);

void FakeTIM_SetInitResult(HAL_StatusTypeDef result);    ///< Drives Init failure
void FakeTIM_SetDeInitResult(HAL_StatusTypeDef result);  ///< Drives Sleep failure
int  FakeTIM_IRQHandlerCallCount(void);                  ///< HAL_TIM_IRQHandler invocation count


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_TIM_H
