/**
 * \file    stm32f4xx_hal_spi.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL SPI surface. Provides
 *          the three SPI instances (backed by real storage so the driver's
 *          handle is non-null) and trivial HAL_SPI_* bodies. C linkage is
 *          preserved on every symbol the driver under test links against.
 *
 * \details HAL_SPI_Init / _DeInit and every transfer entry point return HAL_OK
 *          by default; FakeSPI_SetInitResult / _SetDeInitResult /
 *          _SetTransferResult let a test drive the corresponding failure
 *          branches. HAL_SPI_IRQHandler only counts its invocations, which
 *          proves the SPIx_IRQn vector reached the driver's CallbackIRQ().
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
#include "stm32f4xx_hal_spi.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

SPI_TypeDef s_spi1 = {};
SPI_TypeDef s_spi2 = {};
SPI_TypeDef s_spi3 = {};

HAL_StatusTypeDef s_init_result     = HAL_OK;
HAL_StatusTypeDef s_deinit_result   = HAL_OK;
HAL_StatusTypeDef s_transfer_result = HAL_OK;
int               s_irq_handler_calls = 0;

// Handle of the most recently started IT/DMA transfer; the FakeSPI_Fire*Cplt
// hooks dispatch the matching completion callback against it.
SPI_HandleTypeDef* s_last_handle = nullptr;

} // namespace


/************************************************************************/
/* SPI instances (C linkage to match the driver's view)                */
/************************************************************************/
extern "C" SPI_TypeDef* const SPI1 = &s_spi1;
extern "C" SPI_TypeDef* const SPI2 = &s_spi2;
extern "C" SPI_TypeDef* const SPI3 = &s_spi3;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef* /*hspi*/)
{
    return s_init_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef* /*hspi*/)
{
    return s_deinit_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef* /*hspi*/)
{
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    s_last_handle = hspi;
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef* hspi, uint8_t* /*pTxData*/, uint8_t* /*pRxData*/, uint16_t /*Size*/)
{
    s_last_handle = hspi;
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef* hspi, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    s_last_handle = hspi;
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef* hspi, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    s_last_handle = hspi;
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef* hspi, uint8_t* /*pTxData*/, uint8_t* /*pRxData*/, uint16_t /*Size*/)
{
    s_last_handle = hspi;
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef* hspi, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    s_last_handle = hspi;
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef* /*hspi*/, uint8_t* /*pData*/, uint16_t /*Size*/, uint32_t /*Timeout*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef* /*hspi*/, uint8_t* /*pTxData*/, uint8_t* /*pRxData*/, uint16_t /*Size*/, uint32_t /*Timeout*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef* /*hspi*/, uint8_t* /*pData*/, uint16_t /*Size*/, uint32_t /*Timeout*/)
{
    return s_transfer_result;
}

extern "C" void HAL_SPI_IRQHandler(SPI_HandleTypeDef* /*hspi*/)
{
    ++s_irq_handler_calls;
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeSPI_Reset(void)
{
    s_init_result       = HAL_OK;
    s_deinit_result     = HAL_OK;
    s_transfer_result   = HAL_OK;
    s_irq_handler_calls = 0;
    s_last_handle       = nullptr;
}

extern "C" void FakeSPI_FireTxCplt(void)
{
    if (s_last_handle != nullptr) { HAL_SPI_TxCpltCallback(s_last_handle); }
}

extern "C" void FakeSPI_FireRxCplt(void)
{
    if (s_last_handle != nullptr) { HAL_SPI_RxCpltCallback(s_last_handle); }
}

extern "C" void FakeSPI_FireTxRxCplt(void)
{
    if (s_last_handle != nullptr) { HAL_SPI_TxRxCpltCallback(s_last_handle); }
}

extern "C" void FakeSPI_SetInitResult(HAL_StatusTypeDef result)
{
    s_init_result = result;
}

extern "C" void FakeSPI_SetDeInitResult(HAL_StatusTypeDef result)
{
    s_deinit_result = result;
}

extern "C" void FakeSPI_SetTransferResult(HAL_StatusTypeDef result)
{
    s_transfer_result = result;
}

extern "C" int FakeSPI_IRQHandlerCallCount(void)
{
    return s_irq_handler_calls;
}
