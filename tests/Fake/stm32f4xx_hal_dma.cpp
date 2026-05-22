/**
 * \file    stm32f4xx_hal_dma.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL DMA surface. Provides
 *          the 16 stream instances (backed by real storage so the driver's
 *          handle is non-null) and trivial HAL_DMA_* bodies. C linkage is
 *          preserved on every symbol the driver under test links against.
 *
 * \details HAL_DMA_Init returns HAL_OK by default; FakeDMA_SetInitResult lets a
 *          test drive the Configure()-failure branch. HAL_DMA_IRQHandler only
 *          counts its invocations, which proves the stream IRQ vector reached
 *          the driver's internal Callback().
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
#include <cstring>

extern "C" {
#include "stm32f4xx_hal_dma.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

DMA_Stream_TypeDef s_dma1_streams[8] = {};
DMA_Stream_TypeDef s_dma2_streams[8] = {};

HAL_StatusTypeDef s_init_result      = HAL_OK;
int               s_irq_handler_calls = 0;

} // namespace


/************************************************************************/
/* Stream instances (C linkage to match the driver's view)             */
/************************************************************************/
extern "C" DMA_Stream_TypeDef* const DMA1_Stream0 = &s_dma1_streams[0];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream1 = &s_dma1_streams[1];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream2 = &s_dma1_streams[2];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream3 = &s_dma1_streams[3];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream4 = &s_dma1_streams[4];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream5 = &s_dma1_streams[5];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream6 = &s_dma1_streams[6];
extern "C" DMA_Stream_TypeDef* const DMA1_Stream7 = &s_dma1_streams[7];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream0 = &s_dma2_streams[0];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream1 = &s_dma2_streams[1];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream2 = &s_dma2_streams[2];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream3 = &s_dma2_streams[3];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream4 = &s_dma2_streams[4];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream5 = &s_dma2_streams[5];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream6 = &s_dma2_streams[6];
extern "C" DMA_Stream_TypeDef* const DMA2_Stream7 = &s_dma2_streams[7];


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef* /*hdma*/)
{
    return s_init_result;
}

extern "C" HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef* /*hdma*/)
{
    return HAL_OK;
}

extern "C" void HAL_DMA_IRQHandler(DMA_HandleTypeDef* /*hdma*/)
{
    ++s_irq_handler_calls;
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeDMA_Reset(void)
{
    std::memset(s_dma1_streams, 0, sizeof(s_dma1_streams));
    std::memset(s_dma2_streams, 0, sizeof(s_dma2_streams));
    s_init_result       = HAL_OK;
    s_irq_handler_calls = 0;
}

extern "C" void FakeDMA_SetInitResult(HAL_StatusTypeDef result)
{
    s_init_result = result;
}

extern "C" int FakeDMA_IRQHandlerCallCount(void)
{
    return s_irq_handler_calls;
}
