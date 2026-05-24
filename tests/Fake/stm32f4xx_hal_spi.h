/**
 * \file    stm32f4xx_hal_spi.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL SPI surface for the unit-test build.
 *          Declares the SPI config macros, baud-rate prescalers, RCC SPI clock
 *          gating macros and the HAL_SPI_* prototypes the real SPI driver
 *          compiles against. The SPI types/instances themselves live in the
 *          umbrella stm32f4xx_hal.h (SPI.hpp pulls in only the umbrella).
 *
 * \details The macro values mirror the real HAL so the prescaler the driver
 *          computes carries its true meaning, but no behaviour depends on
 *          them: HAL_SPI_Init returns HAL_OK by default (overridable via
 *          FakeSPI_SetInitResult), the transfer entry points return HAL_OK
 *          (overridable via FakeSPI_SetTransferResult), and HAL_SPI_IRQHandler
 *          only counts invocations to prove the SPIx_IRQn vector dispatched
 *          into the driver's CallbackIRQ().
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_SPI_H
#define __STM32F4xx_HAL_SPI_H


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
#define SPI_MODE_MASTER             ((uint32_t)0x00000104U)
#define SPI_DIRECTION_2LINES        ((uint32_t)0x00000000U)
#define SPI_DATASIZE_8BIT           ((uint32_t)0x00000000U)
#define SPI_POLARITY_LOW            ((uint32_t)0x00000000U)
#define SPI_POLARITY_HIGH           ((uint32_t)0x00000002U)
#define SPI_PHASE_1EDGE             ((uint32_t)0x00000000U)
#define SPI_PHASE_2EDGE             ((uint32_t)0x00000001U)
#define SPI_NSS_SOFT                ((uint32_t)0x00000200U)
#define SPI_FIRSTBIT_MSB            ((uint32_t)0x00000000U)
#define SPI_TIMODE_DISABLE          ((uint32_t)0x00000000U)
#define SPI_CRCCALCULATION_DISABLE  ((uint32_t)0x00000000U)

#define SPI_BAUDRATEPRESCALER_2     ((uint32_t)0x00000000U)
#define SPI_BAUDRATEPRESCALER_4     ((uint32_t)0x00000008U)
#define SPI_BAUDRATEPRESCALER_8     ((uint32_t)0x00000010U)
#define SPI_BAUDRATEPRESCALER_16    ((uint32_t)0x00000018U)
#define SPI_BAUDRATEPRESCALER_32    ((uint32_t)0x00000020U)
#define SPI_BAUDRATEPRESCALER_64    ((uint32_t)0x00000028U)
#define SPI_BAUDRATEPRESCALER_128   ((uint32_t)0x00000030U)
#define SPI_BAUDRATEPRESCALER_256   ((uint32_t)0x00000038U)

#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY               ((uint32_t)0xFFFFFFFFU)
#endif


/************************************************************************/
/* RCC SPI clock gating -- pretend the clocks are already enabled.      */
/************************************************************************/
#define __HAL_RCC_SPI1_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_SPI2_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_SPI3_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_SPI1_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_SPI2_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_SPI3_CLK_DISABLE()   do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_spi.cpp)          */
/************************************************************************/
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef* hspi);
HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef* hspi);
HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef* hspi);

HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);

HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout);

void HAL_SPI_IRQHandler(SPI_HandleTypeDef* hspi);

/* Defined by the driver (SPI.cpp); declared here so the definitions link with C
   linkage and the fake completion-fire hooks below can dispatch into them. On
   hardware the HAL fires these from the IRQ handler at transfer completion. */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi);
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi);
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake SPI state: Init/DeInit/transfer results back to HAL_OK, the
 *  HAL_SPI_IRQHandler call counter to zero, and the last-transfer handle. */
void FakeSPI_Reset(void);

/** Simulate a transfer-complete interrupt on the most recently started IT/DMA
 *  transfer, dispatching into the driver's matching completion callback (which
 *  routes by handle->Instance into the per-instance user handler). Tx for the
 *  Write* paths, Rx for the Read* paths, TxRx for the WriteRead* paths. No-op if
 *  no IT/DMA transfer has been started since the last reset. */
void FakeSPI_FireTxCplt(void);
void FakeSPI_FireRxCplt(void);
void FakeSPI_FireTxRxCplt(void);

/** Force HAL_SPI_Init to return `result`, so the driver's Init()-failure path
 *  (returns false, stays not-initialised) can be exercised. */
void FakeSPI_SetInitResult(HAL_StatusTypeDef result);

/** Force HAL_SPI_DeInit to return `result`, so Sleep()'s failure path can be
 *  exercised. */
void FakeSPI_SetDeInitResult(HAL_StatusTypeDef result);

/** Force every HAL_SPI_Transmit/Receive/TransmitReceive (blocking, IT and DMA)
 *  to return `result`, so the transfer-failure branches can be exercised. */
void FakeSPI_SetTransferResult(HAL_StatusTypeDef result);

/** Number of times HAL_SPI_IRQHandler has been invoked since the last reset --
 *  proves the SPIx_IRQn vector dispatched through CallbackIRQ() into the HAL. */
int FakeSPI_IRQHandlerCallCount(void);


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_SPI_H
