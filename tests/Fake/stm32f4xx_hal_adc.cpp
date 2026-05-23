/**
 * \file    stm32f4xx_hal_adc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL ADC surface. Provides
 *          the three ADC instances (backed by real storage so the driver's
 *          handle is non-null) and trivial HAL_ADC_* bodies. C linkage is
 *          preserved on every symbol the driver under test links against.
 *
 * \details Each entry point returns its own controllable result (HAL_OK by
 *          default) so the driver's per-operation failure branches can be
 *          exercised in isolation. HAL_ADC_GetValue returns a controllable
 *          conversion value. HAL_ADC_IRQHandler counts its calls and dispatches
 *          into the driver's HAL_ADC_ConvCpltCallback override, modelling an
 *          end-of-conversion interrupt so the GetValueInterrupt callback path is
 *          reachable natively. Stop / Stop_IT report HAL_OK unless the matching
 *          Set*Result hook overrides Stop.
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
#include "stm32f4xx_hal_adc.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

ADC_TypeDef s_adc1 = {};
ADC_TypeDef s_adc2 = {};
ADC_TypeDef s_adc3 = {};

HAL_StatusTypeDef s_init_result     = HAL_OK;
HAL_StatusTypeDef s_deinit_result   = HAL_OK;
HAL_StatusTypeDef s_config_result   = HAL_OK;
HAL_StatusTypeDef s_start_result    = HAL_OK;
HAL_StatusTypeDef s_stop_result     = HAL_OK;
HAL_StatusTypeDef s_poll_result     = HAL_OK;
HAL_StatusTypeDef s_start_it_result = HAL_OK;
uint32_t          s_conversion_value = 0;
int               s_irq_handler_calls = 0;

} // namespace


/************************************************************************/
/* ADC instances (C linkage to match the driver's view)                */
/************************************************************************/
extern "C" ADC_TypeDef* const ADC1 = &s_adc1;
extern "C" ADC_TypeDef* const ADC2 = &s_adc2;
extern "C" ADC_TypeDef* const ADC3 = &s_adc3;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* /*hadc*/)   { return s_init_result; }
extern "C" HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* /*hadc*/) { return s_deinit_result; }

extern "C" HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* /*hadc*/, ADC_ChannelConfTypeDef* /*sConfig*/)
{
    return s_config_result;
}

extern "C" HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* /*hadc*/) { return s_start_result; }
extern "C" HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* /*hadc*/)  { return s_stop_result; }

extern "C" HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* /*hadc*/, uint32_t /*Timeout*/)
{
    return s_poll_result;
}

extern "C" uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* /*hadc*/) { return s_conversion_value; }

extern "C" HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* /*hadc*/) { return s_start_it_result; }
extern "C" HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* /*hadc*/)  { return HAL_OK; }

extern "C" void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc)
{
    ++s_irq_handler_calls;

    // On hardware HAL_ADC_IRQHandler resolves the EOC flag and invokes the
    // conversion-complete callback; model that unconditionally here so the
    // driver's end-of-conversion dispatch is reachable from a fired vector.
    HAL_ADC_ConvCpltCallback(hadc);
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeADC_Reset(void)
{
    s_init_result      = HAL_OK;
    s_deinit_result    = HAL_OK;
    s_config_result    = HAL_OK;
    s_start_result     = HAL_OK;
    s_stop_result      = HAL_OK;
    s_poll_result      = HAL_OK;
    s_start_it_result  = HAL_OK;
    s_conversion_value = 0;
    s_irq_handler_calls = 0;
}

extern "C" void FakeADC_SetInitResult(HAL_StatusTypeDef result)          { s_init_result = result; }
extern "C" void FakeADC_SetDeInitResult(HAL_StatusTypeDef result)        { s_deinit_result = result; }
extern "C" void FakeADC_SetConfigChannelResult(HAL_StatusTypeDef result) { s_config_result = result; }
extern "C" void FakeADC_SetStartResult(HAL_StatusTypeDef result)         { s_start_result = result; }
extern "C" void FakeADC_SetStopResult(HAL_StatusTypeDef result)          { s_stop_result = result; }
extern "C" void FakeADC_SetPollResult(HAL_StatusTypeDef result)          { s_poll_result = result; }
extern "C" void FakeADC_SetStartItResult(HAL_StatusTypeDef result)       { s_start_it_result = result; }
extern "C" void FakeADC_SetConversionValue(uint32_t value)               { s_conversion_value = value; }
extern "C" int  FakeADC_IRQHandlerCallCount(void)                        { return s_irq_handler_calls; }
