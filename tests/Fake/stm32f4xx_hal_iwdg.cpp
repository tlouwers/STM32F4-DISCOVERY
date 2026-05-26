/**
 * \file    stm32f4xx_hal_iwdg.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL IWDG surface. Provides
 *          the single IWDG instance (backed by real storage so the driver's
 *          handle is non-null), the controllable LSI-ready flag, and the
 *          HAL_IWDG_Init / HAL_IWDG_Refresh bodies. C linkage is preserved on
 *          every symbol the driver under test links against.
 *
 * \details HAL_IWDG_Init returns its controllable result (HAL_OK by default).
 *          FakeRCC_GetFlag reports the fake LSI-ready state for RCC_FLAG_LSIRDY
 *          (ready by default). HAL_IWDG_Refresh counts its calls since the
 *          driver's Refresh() discards the return value.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
extern "C" {
#include "stm32f4xx_hal_iwdg.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

IWDG_TypeDef s_iwdg = {};

HAL_StatusTypeDef s_init_result   = HAL_OK;
int               s_lsi_ready      = 1;
int               s_refresh_calls  = 0;

} // namespace


/************************************************************************/
/* IWDG instance (C linkage to match the driver's view)                 */
/************************************************************************/
extern "C" IWDG_TypeDef* const IWDG = &s_iwdg;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" uint32_t FakeRCC_GetFlag(uint32_t /*flag*/)
{
    // The driver only ever queries RCC_FLAG_LSIRDY.
    return (s_lsi_ready != 0) ? 1U : 0U;
}

extern "C" HAL_StatusTypeDef HAL_IWDG_Init(IWDG_HandleTypeDef* /*hiwdg*/) { return s_init_result; }

extern "C" HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef* /*hiwdg*/)
{
    ++s_refresh_calls;
    return HAL_OK;
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeIWDG_Reset(void)
{
    s_init_result  = HAL_OK;
    s_lsi_ready     = 1;
    s_refresh_calls = 0;
}

extern "C" void FakeIWDG_SetInitResult(HAL_StatusTypeDef result) { s_init_result = result; }
extern "C" void FakeIWDG_SetLSIReady(int ready)                  { s_lsi_ready = ready; }
extern "C" int  FakeIWDG_RefreshCallCount(void)                  { return s_refresh_calls; }
