/**
 * \file    stm32f4xx_hal_dac.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL DAC surface. Provides
 *          the single DAC instance (backed by real storage so the driver's
 *          handle is non-null) and trivial HAL_DAC_* bodies. C linkage is
 *          preserved on every symbol the driver under test links against.
 *
 * \details Each entry point returns its own controllable result (HAL_OK by
 *          default) so the driver's per-operation failure branches can be
 *          exercised in isolation. Stop / Stop_DMA are no-op HAL_OK (the driver
 *          ignores their result).
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
#include "stm32f4xx_hal_dac.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

DAC_TypeDef s_dac = {};

HAL_StatusTypeDef s_init_result      = HAL_OK;
HAL_StatusTypeDef s_deinit_result    = HAL_OK;
HAL_StatusTypeDef s_config_result    = HAL_OK;
HAL_StatusTypeDef s_start_result     = HAL_OK;
HAL_StatusTypeDef s_setvalue_result  = HAL_OK;
HAL_StatusTypeDef s_start_dma_result = HAL_OK;

} // namespace


/************************************************************************/
/* DAC instance (C linkage to match the driver's view)                 */
/************************************************************************/
extern "C" DAC_TypeDef* const DAC = &s_dac;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef* /*hdac*/)   { return s_init_result; }
extern "C" HAL_StatusTypeDef HAL_DAC_DeInit(DAC_HandleTypeDef* /*hdac*/) { return s_deinit_result; }

extern "C" HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef* /*hdac*/, DAC_ChannelConfTypeDef* /*sConfig*/, uint32_t /*Channel*/)
{
    return s_config_result;
}

extern "C" HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef* /*hdac*/, uint32_t /*Channel*/, uint32_t /*Alignment*/, uint32_t /*Data*/)
{
    return s_setvalue_result;
}

extern "C" HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef* /*hdac*/, uint32_t /*Channel*/)
{
    return s_start_result;
}

extern "C" HAL_StatusTypeDef HAL_DAC_Stop(DAC_HandleTypeDef* /*hdac*/, uint32_t /*Channel*/)
{
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef HAL_DAC_Start_DMA(DAC_HandleTypeDef* /*hdac*/, uint32_t /*Channel*/, uint32_t* /*pData*/, uint32_t /*Length*/, uint32_t /*Alignment*/)
{
    return s_start_dma_result;
}

extern "C" HAL_StatusTypeDef HAL_DAC_Stop_DMA(DAC_HandleTypeDef* /*hdac*/, uint32_t /*Channel*/)
{
    return HAL_OK;
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeDAC_Reset(void)
{
    s_init_result      = HAL_OK;
    s_deinit_result    = HAL_OK;
    s_config_result    = HAL_OK;
    s_start_result     = HAL_OK;
    s_setvalue_result  = HAL_OK;
    s_start_dma_result = HAL_OK;
}

extern "C" void FakeDAC_SetInitResult(HAL_StatusTypeDef result)          { s_init_result = result; }
extern "C" void FakeDAC_SetDeInitResult(HAL_StatusTypeDef result)        { s_deinit_result = result; }
extern "C" void FakeDAC_SetConfigChannelResult(HAL_StatusTypeDef result) { s_config_result = result; }
extern "C" void FakeDAC_SetStartResult(HAL_StatusTypeDef result)         { s_start_result = result; }
extern "C" void FakeDAC_SetSetValueResult(HAL_StatusTypeDef result)      { s_setvalue_result = result; }
extern "C" void FakeDAC_SetStartDmaResult(HAL_StatusTypeDef result)      { s_start_dma_result = result; }
