/**
 * \file    stm32f4xx_hal_i2c.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL I2C surface for the unit-test build.
 *          Declares the I2C config macros, CR1 bits, the register read/modify
 *          helpers, RCC I2C clock gating macros and the HAL_I2C_* prototypes
 *          the real I2C driver compiles against. The I2C types/instances
 *          themselves live in the umbrella stm32f4xx_hal.h (I2C.hpp pulls in
 *          only the umbrella).
 *
 * \details HAL_I2C_Init / _DeInit and every master transfer entry point return
 *          HAL_OK by default; FakeI2C_SetInitResult / _SetDeInitResult /
 *          _SetTransferResult drive the failure branches. HAL_I2C_GetState
 *          returns HAL_I2C_STATE_READY by default; FakeI2C_SetState drives the
 *          Sleep() abort branch. HAL_I2C_EV/ER_IRQHandler only count
 *          invocations, proving the I2Cx_EV/ER_IRQn vectors dispatched through
 *          the driver's CallbackEvent/CallbackError into the HAL.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_I2C_H
#define __STM32F4xx_HAL_I2C_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \brief   I2C state -- minimal subset. Sleep() reads this to decide whether an
 *          in-flight master transfer must be aborted before DeInit.
 */
typedef enum
{
    HAL_I2C_STATE_RESET   = 0x00U,
    HAL_I2C_STATE_READY   = 0x20U,
    HAL_I2C_STATE_BUSY    = 0x24U,
    HAL_I2C_STATE_BUSY_TX = 0x21U,
    HAL_I2C_STATE_BUSY_RX = 0x22U
} HAL_I2C_StateTypeDef;


/************************************************************************/
/* Config / register-value macros (mirror the real HAL)                 */
/************************************************************************/
#define I2C_DUTYCYCLE_2             ((uint32_t)0x00000000U)
#define I2C_DUTYCYCLE_16_9          ((uint32_t)0x00004000U)
#define I2C_ADDRESSINGMODE_7BIT     ((uint32_t)0x00004000U)
#define I2C_DUALADDRESS_DISABLE     ((uint32_t)0x00000000U)
#define I2C_GENERALCALL_DISABLE     ((uint32_t)0x00000000U)
#define I2C_NOSTRETCH_DISABLE       ((uint32_t)0x00000000U)

#define I2C_CR1_PE                  ((uint32_t)0x00000001U)   ///< Peripheral enable
#define I2C_CR1_SWRST               ((uint32_t)0x00008000U)   ///< Software reset

#define HAL_I2C_ERROR_NONE          ((uint32_t)0x00000000U)

#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY               ((uint32_t)0xFFFFFFFFU)
#endif


/************************************************************************/
/* Register read/modify helpers (mirror stm32f4xx.h)                    */
/************************************************************************/
#ifndef SET_BIT
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#endif
#ifndef CLEAR_BIT
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#endif

/** Disable the I2C peripheral (clear CR1.PE). */
#define __HAL_I2C_DISABLE(__HANDLE__)   ((__HANDLE__)->Instance->CR1 &= ~I2C_CR1_PE)


/************************************************************************/
/* RCC I2C clock gating -- pretend the clocks are already enabled.      */
/************************************************************************/
#define __HAL_RCC_I2C1_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_I2C2_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_I2C3_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_I2C1_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_I2C2_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_I2C3_CLK_DISABLE()   do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_i2c.cpp)          */
/************************************************************************/
HAL_StatusTypeDef    HAL_I2C_Init(I2C_HandleTypeDef* hi2c);
HAL_StatusTypeDef    HAL_I2C_DeInit(I2C_HandleTypeDef* hi2c);
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef* hi2c);
HAL_StatusTypeDef    HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress);

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Master_Receive_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);

void HAL_I2C_EV_IRQHandler(I2C_HandleTypeDef* hi2c);
void HAL_I2C_ER_IRQHandler(I2C_HandleTypeDef* hi2c);

/* Defined by the driver (I2C.cpp); declared here for C linkage so the fake's
   completion-fire hooks below can dispatch into them. On hardware the HAL fires
   these from the EV/ER IRQ handlers at transfer completion / error / abort. */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c);
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef* hi2c);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake I2C state: Init/DeInit/transfer results to HAL_OK, the
 *  reported state to HAL_I2C_STATE_READY, the EV/ER IRQ counters to zero, and
 *  the last-transfer handle. */
void FakeI2C_Reset(void);

/** Simulate completion of the most recently started IT/DMA master transfer,
 *  dispatching into the driver's matching callback (routes by handle->Instance
 *  into the per-instance user handler). TxCplt/RxCplt fire success=true; Error
 *  and Abort fire both Tx+Rx slots success=false. No-op if no IT/DMA transfer
 *  has been started since the last reset. */
void FakeI2C_FireMasterTxCplt(void);
void FakeI2C_FireMasterRxCplt(void);
void FakeI2C_FireError(void);
void FakeI2C_FireAbort(void);

/** Force HAL_I2C_Init to return `result` (drives the Init / RecoverBus re-init
 *  failure paths). */
void FakeI2C_SetInitResult(HAL_StatusTypeDef result);

/** Force HAL_I2C_DeInit to return `result` (drives the Sleep failure path). */
void FakeI2C_SetDeInitResult(HAL_StatusTypeDef result);

/** Force every master transfer (blocking / IT / DMA) to return `result`. */
void FakeI2C_SetTransferResult(HAL_StatusTypeDef result);

/** Set the state HAL_I2C_GetState reports, so Sleep()'s abort branch
 *  (BUSY_TX / BUSY_RX) can be exercised. */
void FakeI2C_SetState(HAL_I2C_StateTypeDef state);

/** Number of HAL_I2C_Master_Abort_IT calls since the last reset. */
int FakeI2C_AbortCallCount(void);

/** Number of HAL_I2C_EV_IRQHandler calls since the last reset. */
int FakeI2C_EvIRQHandlerCallCount(void);

/** Number of HAL_I2C_ER_IRQHandler calls since the last reset. */
int FakeI2C_ErIRQHandlerCallCount(void);


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_I2C_H
