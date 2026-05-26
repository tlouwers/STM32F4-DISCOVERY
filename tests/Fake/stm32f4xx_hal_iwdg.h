/**
 * \file    stm32f4xx_hal_iwdg.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL IWDG surface for the unit-test build.
 *          Declares the prescaler macros, the LSI-ready RCC flag query the
 *          driver gates Init on, the debug-freeze macro, and the HAL_IWDG_*
 *          prototypes the real Watchdog driver compiles against. The IWDG
 *          types/instance themselves live in the umbrella stm32f4xx_hal.h
 *          (Watchdog.hpp pulls in only the umbrella for its handle member).
 *
 * \details HAL_IWDG_Init returns HAL_OK by default; FakeIWDG_SetInitResult drives
 *          the failure branch. __HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) routes to a
 *          controllable fake (FakeIWDG_SetLSIReady) so the driver's LSI-not-ready
 *          guard can be exercised. HAL_IWDG_Refresh counts its invocations
 *          (FakeIWDG_RefreshCallCount) since the driver's Refresh() is void.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_IWDG_H
#define __STM32F4xx_HAL_IWDG_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Prescaler macros (mirror the real HAL)                               */
/************************************************************************/
#define IWDG_PRESCALER_4     ((uint32_t)0x00000000U)
#define IWDG_PRESCALER_8     ((uint32_t)0x00000001U)
#define IWDG_PRESCALER_16    ((uint32_t)0x00000002U)
#define IWDG_PRESCALER_32    ((uint32_t)0x00000003U)
#define IWDG_PRESCALER_64    ((uint32_t)0x00000004U)
#define IWDG_PRESCALER_128   ((uint32_t)0x00000005U)
#define IWDG_PRESCALER_256   ((uint32_t)0x00000006U)


/************************************************************************/
/* RCC LSI-ready flag query + debug freeze (mirror the real HAL)        */
/************************************************************************/
#define RCC_FLAG_LSIRDY      ((uint32_t)0x00000061U)

/* The driver gates Init on the LSI clock being ready; route the flag query to a
   controllable fake so both the ready and not-ready paths are testable. */
uint32_t FakeRCC_GetFlag(uint32_t flag);
#define __HAL_RCC_GET_FLAG(__FLAG__)   FakeRCC_GetFlag(__FLAG__)

/* Freezing the IWDG while the debugger is halted has no native effect. */
#define __HAL_DBGMCU_FREEZE_IWDG()     do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_iwdg.cpp)         */
/************************************************************************/
HAL_StatusTypeDef HAL_IWDG_Init(IWDG_HandleTypeDef* hiwdg);
HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef* hiwdg);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake IWDG state: Init result to HAL_OK, LSI ready, refresh count 0. */
void FakeIWDG_Reset(void);

void FakeIWDG_SetInitResult(HAL_StatusTypeDef result);   ///< Drives Init failure
void FakeIWDG_SetLSIReady(int ready);                    ///< Drives the LSI-not-ready Init guard
int  FakeIWDG_RefreshCallCount(void);                    ///< HAL_IWDG_Refresh invocation count


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_IWDG_H
