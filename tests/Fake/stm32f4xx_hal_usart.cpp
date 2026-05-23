/**
 * \file    stm32f4xx_hal_usart.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL UART surface. Provides
 *          the four USART instances (backed by real storage so the driver's
 *          handle is non-null and SR/DR have somewhere to live) and trivial
 *          HAL_UART_* bodies. C linkage is preserved on every symbol the driver
 *          under test links against.
 *
 * \details The transfer entry points reject a null buffer or zero length with
 *          HAL_ERROR (mirroring the real HAL, which the driver relies on for
 *          those guards) and otherwise return the controllable transfer result.
 *          HAL_UART_IRQHandler only counts its invocations, proving the
 *          USARTx_IRQn vector reached the driver's CallbackIRQ().
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
#include "stm32f4xx_hal_usart.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

USART_TypeDef s_usart1 = {};
USART_TypeDef s_usart2 = {};
USART_TypeDef s_usart3 = {};
USART_TypeDef s_usart6 = {};

HAL_StatusTypeDef s_init_result     = HAL_OK;
HAL_StatusTypeDef s_deinit_result   = HAL_OK;
HAL_StatusTypeDef s_transfer_result = HAL_OK;
int               s_irq_handler_calls = 0;

// Mirror the real HAL's parameter rejection so the driver's null/zero paths,
// which delegate to the HAL, return false as documented on IUSART.
HAL_StatusTypeDef TransferResult(const void* buf, uint16_t size)
{
    if ((buf == nullptr) || (size == 0U)) { return HAL_ERROR; }
    return s_transfer_result;
}

} // namespace


/************************************************************************/
/* USART instances (C linkage to match the driver's view)              */
/************************************************************************/
extern "C" USART_TypeDef* const USART1 = &s_usart1;
extern "C" USART_TypeDef* const USART2 = &s_usart2;
extern "C" USART_TypeDef* const USART3 = &s_usart3;
extern "C" USART_TypeDef* const USART6 = &s_usart6;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef* /*huart*/)   { return s_init_result; }
extern "C" HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef* /*huart*/) { return s_deinit_result; }
extern "C" HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef* /*huart*/)  { return HAL_OK; }
extern "C" HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef* /*huart*/) { return HAL_OK; }

extern "C" HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* /*huart*/, uint8_t* pData, uint16_t Size, uint32_t /*Timeout*/)
{
    return TransferResult(pData, Size);
}

extern "C" HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* /*huart*/, uint8_t* pData, uint16_t Size, uint32_t /*Timeout*/)
{
    return TransferResult(pData, Size);
}

extern "C" HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef* /*huart*/, uint8_t* pData, uint16_t Size)
{
    return TransferResult(pData, Size);
}

extern "C" HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef* /*huart*/, uint8_t* pData, uint16_t Size)
{
    return TransferResult(pData, Size);
}

extern "C" HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* /*huart*/, uint8_t* pData, uint16_t Size)
{
    return TransferResult(pData, Size);
}

extern "C" HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* /*huart*/, uint8_t* pData, uint16_t Size)
{
    return TransferResult(pData, Size);
}

extern "C" void HAL_UART_IRQHandler(UART_HandleTypeDef* /*huart*/)
{
    ++s_irq_handler_calls;
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeUSART_Reset(void)
{
    s_usart1 = USART_TypeDef{};
    s_usart2 = USART_TypeDef{};
    s_usart3 = USART_TypeDef{};
    s_usart6 = USART_TypeDef{};
    s_init_result       = HAL_OK;
    s_deinit_result     = HAL_OK;
    s_transfer_result   = HAL_OK;
    s_irq_handler_calls = 0;
}

extern "C" void FakeUSART_SetInitResult(HAL_StatusTypeDef result)     { s_init_result = result; }
extern "C" void FakeUSART_SetDeInitResult(HAL_StatusTypeDef result)   { s_deinit_result = result; }
extern "C" void FakeUSART_SetTransferResult(HAL_StatusTypeDef result) { s_transfer_result = result; }
extern "C" int  FakeUSART_IRQHandlerCallCount(void)                   { return s_irq_handler_calls; }
