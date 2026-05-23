/**
 * \file    stm32f4xx_hal_tim.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL TIM (time-base)
 *          surface. Provides the fourteen TIM instances (backed by real storage
 *          so a driver's handle is non-null) and trivial HAL_TIM_Base_* bodies.
 *          C linkage is preserved on every symbol the driver under test links
 *          against. Shared by every timer driver + the TimerIRQ dispatcher.
 *
 * \details HAL_TIM_Base_Init / _DeInit return their controllable result (HAL_OK
 *          by default); FakeTIM_SetInitResult / _SetDeInitResult drive those
 *          failure branches. Start / Stop (and their _IT variants) are no-op
 *          HAL_OK -- the drivers ignore their result. HAL_TIM_IRQHandler counts
 *          its calls and dispatches into the driver's
 *          HAL_TIM_PeriodElapsedCallback override, modelling a period-elapsed
 *          interrupt so the elapsed-callback path is reachable natively once a
 *          TimerIRQ slot has routed the fired vector into the driver.
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
#include "stm32f4xx_hal_tim.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

TIM_TypeDef s_tim1  = {};
TIM_TypeDef s_tim2  = {};
TIM_TypeDef s_tim3  = {};
TIM_TypeDef s_tim4  = {};
TIM_TypeDef s_tim5  = {};
TIM_TypeDef s_tim6  = {};
TIM_TypeDef s_tim7  = {};
TIM_TypeDef s_tim8  = {};
TIM_TypeDef s_tim9  = {};
TIM_TypeDef s_tim10 = {};
TIM_TypeDef s_tim11 = {};
TIM_TypeDef s_tim12 = {};
TIM_TypeDef s_tim13 = {};
TIM_TypeDef s_tim14 = {};

HAL_StatusTypeDef s_init_result          = HAL_OK;
HAL_StatusTypeDef s_deinit_result        = HAL_OK;
HAL_StatusTypeDef s_master_config_result = HAL_OK;
int               s_irq_handler_calls    = 0;

HAL_StatusTypeDef s_pwm_init_result    = HAL_OK;
HAL_StatusTypeDef s_pwm_deinit_result  = HAL_OK;
HAL_StatusTypeDef s_pwm_config_result  = HAL_OK;
HAL_StatusTypeDef s_pwm_start_result   = HAL_OK;
HAL_StatusTypeDef s_pwm_stop_result    = HAL_OK;

} // namespace


/************************************************************************/
/* TIM instances (C linkage to match the driver's view)                */
/************************************************************************/
extern "C" TIM_TypeDef* const TIM1  = &s_tim1;
extern "C" TIM_TypeDef* const TIM2  = &s_tim2;
extern "C" TIM_TypeDef* const TIM3  = &s_tim3;
extern "C" TIM_TypeDef* const TIM4  = &s_tim4;
extern "C" TIM_TypeDef* const TIM5  = &s_tim5;
extern "C" TIM_TypeDef* const TIM6  = &s_tim6;
extern "C" TIM_TypeDef* const TIM7  = &s_tim7;
extern "C" TIM_TypeDef* const TIM8  = &s_tim8;
extern "C" TIM_TypeDef* const TIM9  = &s_tim9;
extern "C" TIM_TypeDef* const TIM10 = &s_tim10;
extern "C" TIM_TypeDef* const TIM11 = &s_tim11;
extern "C" TIM_TypeDef* const TIM12 = &s_tim12;
extern "C" TIM_TypeDef* const TIM13 = &s_tim13;
extern "C" TIM_TypeDef* const TIM14 = &s_tim14;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef* /*htim*/)   { return s_init_result; }
extern "C" HAL_StatusTypeDef HAL_TIM_Base_DeInit(TIM_HandleTypeDef* /*htim*/) { return s_deinit_result; }

extern "C" HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef* /*htim*/)    { return HAL_OK; }
extern "C" HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef* /*htim*/)     { return HAL_OK; }
extern "C" HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef* /*htim*/) { return HAL_OK; }
extern "C" HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef* /*htim*/)  { return HAL_OK; }

extern "C" HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef* /*htim*/, TIM_MasterConfigTypeDef* /*sMasterConfig*/)
{
    return s_master_config_result;
}

extern "C" void HAL_TIM_IRQHandler(TIM_HandleTypeDef* htim)
{
    ++s_irq_handler_calls;

    // On hardware HAL_TIM_IRQHandler resolves the update flag and invokes the
    // period-elapsed callback; model that unconditionally here so the driver's
    // elapsed dispatch is reachable from a fired vector.
    HAL_TIM_PeriodElapsedCallback(htim);
}

extern "C" HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef* /*htim*/)   { return s_pwm_init_result; }
extern "C" HAL_StatusTypeDef HAL_TIM_PWM_DeInit(TIM_HandleTypeDef* /*htim*/) { return s_pwm_deinit_result; }

extern "C" HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef* /*htim*/, TIM_OC_InitTypeDef* /*sConfig*/, uint32_t /*Channel*/)
{
    return s_pwm_config_result;
}

extern "C" HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef* /*htim*/, uint32_t /*Channel*/) { return s_pwm_start_result; }
extern "C" HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef* /*htim*/, uint32_t /*Channel*/)  { return s_pwm_stop_result; }


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeTIM_Reset(void)
{
    s_init_result          = HAL_OK;
    s_deinit_result        = HAL_OK;
    s_master_config_result = HAL_OK;
    s_irq_handler_calls    = 0;

    s_pwm_init_result   = HAL_OK;
    s_pwm_deinit_result = HAL_OK;
    s_pwm_config_result = HAL_OK;
    s_pwm_start_result  = HAL_OK;
    s_pwm_stop_result   = HAL_OK;
}

extern "C" void FakeTIM_SetInitResult(HAL_StatusTypeDef result)          { s_init_result = result; }
extern "C" void FakeTIM_SetDeInitResult(HAL_StatusTypeDef result)        { s_deinit_result = result; }
extern "C" void FakeTIM_SetMasterConfigResult(HAL_StatusTypeDef result)  { s_master_config_result = result; }
extern "C" int  FakeTIM_IRQHandlerCallCount(void)                        { return s_irq_handler_calls; }

extern "C" void FakeTIM_SetPwmInitResult(HAL_StatusTypeDef result)          { s_pwm_init_result = result; }
extern "C" void FakeTIM_SetPwmDeInitResult(HAL_StatusTypeDef result)        { s_pwm_deinit_result = result; }
extern "C" void FakeTIM_SetPwmConfigChannelResult(HAL_StatusTypeDef result) { s_pwm_config_result = result; }
extern "C" void FakeTIM_SetPwmStartResult(HAL_StatusTypeDef result)         { s_pwm_start_result = result; }
extern "C" void FakeTIM_SetPwmStopResult(HAL_StatusTypeDef result)          { s_pwm_stop_result = result; }
