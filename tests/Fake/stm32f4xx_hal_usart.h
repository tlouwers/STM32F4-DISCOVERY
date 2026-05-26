/**
 * \file    stm32f4xx_hal_usart.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL UART surface for the unit-test build.
 *          Declares the UART config macros, the IDLE flag / interrupt-enable
 *          macros and the HAL_UART_* prototypes the real USART driver compiles
 *          against. The UART types/instances themselves live in the umbrella
 *          stm32f4xx_hal.h (USART.hpp pulls in only the umbrella).
 *
 * \details HAL_UART_Init / _DeInit return HAL_OK by default (overridable via
 *          FakeUSART_SetInitResult / _SetDeInitResult). The transfer entry
 *          points reject a null buffer or zero length with HAL_ERROR exactly as
 *          the real HAL does (the driver delegates those guards to the HAL),
 *          and otherwise return FakeUSART_SetTransferResult's value.
 *          HAL_UART_IRQHandler only counts invocations, proving the USARTx_IRQn
 *          vector dispatched through CallbackIRQ into the HAL. The default
 *          status register reads 0, so CallbackIRQ takes the non-IDLE path.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_USART_H
#define __STM32F4xx_HAL_USART_H


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
#define UART_WORDLENGTH_8B          ((uint32_t)0x00000000U)
#define UART_WORDLENGTH_9B          ((uint32_t)0x00001000U)
#define UART_STOPBITS_1             ((uint32_t)0x00000000U)
#define UART_STOPBITS_2             ((uint32_t)0x00002000U)
#define UART_PARITY_NONE            ((uint32_t)0x00000000U)
#define UART_PARITY_EVEN            ((uint32_t)0x00000400U)
#define UART_PARITY_ODD             ((uint32_t)0x00000600U)
#define UART_MODE_TX_RX             ((uint32_t)0x0000000CU)
#define UART_HWCONTROL_NONE         ((uint32_t)0x00000000U)
#define UART_HWCONTROL_RTS_CTS      ((uint32_t)0x00000300U)
#define UART_OVERSAMPLING_16        ((uint32_t)0x00000000U)
#define UART_OVERSAMPLING_8         ((uint32_t)0x00008000U)

#define UART_FLAG_IDLE              ((uint32_t)0x00000010U)   ///< SR bit 4: IDLE line detected
#define UART_IT_IDLE                ((uint32_t)0x00000010U)

#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY               ((uint32_t)0xFFFFFFFFU)
#endif


/************************************************************************/
/* Flag / interrupt macros                                              */
/************************************************************************/
/** Read a status-register flag (default SR == 0, so IDLE reads false). */
#define __HAL_UART_GET_FLAG(__HANDLE__, __FLAG__) \
    (((__HANDLE__)->Instance->SR & (__FLAG__)) == (__FLAG__))

/** Clear a status-register flag. */
#define __HAL_UART_CLEAR_FLAG(__HANDLE__, __FLAG__) \
    ((__HANDLE__)->Instance->SR &= ~(__FLAG__))

/* Interrupt enable/disable -- no-ops in the fake (no NVIC, nothing observes
   them). The real macros twiddle CR1/CR2/CR3 by interrupt encoding. */
#define __HAL_UART_ENABLE_IT(__HANDLE__, __INTERRUPT__)   do { (void)(__HANDLE__); (void)(__INTERRUPT__); } while(0)
#define __HAL_UART_DISABLE_IT(__HANDLE__, __INTERRUPT__)  do { (void)(__HANDLE__); (void)(__INTERRUPT__); } while(0)


/************************************************************************/
/* RCC USART clock gating -- pretend the clocks are already enabled.    */
/************************************************************************/
#define __HAL_RCC_USART1_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_USART2_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_USART3_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_USART6_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_USART1_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_USART2_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_USART3_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_USART6_CLK_DISABLE()   do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_usart.cpp)        */
/************************************************************************/
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef* huart);
HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef* huart);
HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef* huart);
HAL_StatusTypeDef HAL_UART_AbortReceive_IT(UART_HandleTypeDef* huart);

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);

void HAL_UART_IRQHandler(UART_HandleTypeDef* huart);

/* Completion callbacks: the driver provides the definitions (they are __weak in
   the real HAL); declared here so the driver's call sites compile. */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake UART state: Init/DeInit/transfer results to HAL_OK and the
 *  HAL_UART_IRQHandler call counter to zero. */
void FakeUSART_Reset(void);

/** Force HAL_UART_Init to return `result` (drives the Init failure path). */
void FakeUSART_SetInitResult(HAL_StatusTypeDef result);

/** Force HAL_UART_DeInit to return `result` (drives the Sleep failure path). */
void FakeUSART_SetDeInitResult(HAL_StatusTypeDef result);

/** Force every transfer (blocking / IT / DMA) to return `result` -- applied
 *  only after the null/zero-length check, which always yields HAL_ERROR. */
void FakeUSART_SetTransferResult(HAL_StatusTypeDef result);

/** Number of HAL_UART_IRQHandler calls since the last reset. */
int FakeUSART_IRQHandlerCallCount(void);

/** Simulate a Tx-complete interrupt on the most recently started IT/DMA
 *  transfer, dispatching into the driver's HAL_UART_TxCpltCallback (routes by
 *  handle->Instance into the per-instance user handler). No-op if none started
 *  since the last reset. */
void FakeUSART_FireTxCplt(void);

/** Set (set != 0) or clear the IDLE-line flag on every USART status register, so
 *  that firing a USARTx vector takes CallbackIRQ's IDLE path into
 *  HAL_UART_RxCpltCallback. The Receive IT/DMA entry points pre-load RxXferSize
 *  with the requested size so that path reports a non-zero byte count. */
void FakeUSART_SetIdleFlag(int set);


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_USART_H
