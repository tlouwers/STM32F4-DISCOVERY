/**
 * \file    stm32f4xx_hal_i2c.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL I2C surface. Provides
 *          the three I2C instances (backed by real storage so the driver's
 *          handle is non-null and CR1 has somewhere to write) and trivial
 *          HAL_I2C_* bodies. C linkage is preserved on every symbol the driver
 *          under test links against.
 *
 * \details Init/DeInit/transfer results and the reported state are all
 *          controllable via the FakeI2C_* hooks so the driver's failure and
 *          abort branches can be exercised. HAL_I2C_EV/ER_IRQHandler and
 *          HAL_I2C_Master_Abort_IT only count their invocations.
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
#include "stm32f4xx_hal_i2c.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

I2C_TypeDef s_i2c1 = {};
I2C_TypeDef s_i2c2 = {};
I2C_TypeDef s_i2c3 = {};

HAL_StatusTypeDef    s_init_result     = HAL_OK;
HAL_StatusTypeDef    s_deinit_result   = HAL_OK;
HAL_StatusTypeDef    s_transfer_result = HAL_OK;
HAL_I2C_StateTypeDef s_state           = HAL_I2C_STATE_READY;

int s_abort_calls = 0;
int s_ev_irq_calls = 0;
int s_er_irq_calls = 0;

} // namespace


/************************************************************************/
/* I2C instances (C linkage to match the driver's view)                */
/************************************************************************/
extern "C" I2C_TypeDef* const I2C1 = &s_i2c1;
extern "C" I2C_TypeDef* const I2C2 = &s_i2c2;
extern "C" I2C_TypeDef* const I2C3 = &s_i2c3;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef* /*hi2c*/)
{
    return s_init_result;
}

extern "C" HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef* /*hi2c*/)
{
    return s_deinit_result;
}

extern "C" HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef* /*hi2c*/)
{
    return s_state;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/)
{
    ++s_abort_calls;
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/, uint8_t* /*pData*/, uint16_t /*Size*/, uint32_t /*Timeout*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/, uint8_t* /*pData*/, uint16_t /*Size*/, uint32_t /*Timeout*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    return s_transfer_result;
}

extern "C" HAL_StatusTypeDef HAL_I2C_Master_Receive_DMA(I2C_HandleTypeDef* /*hi2c*/, uint16_t /*DevAddress*/, uint8_t* /*pData*/, uint16_t /*Size*/)
{
    return s_transfer_result;
}

extern "C" void HAL_I2C_EV_IRQHandler(I2C_HandleTypeDef* /*hi2c*/)
{
    ++s_ev_irq_calls;
}

extern "C" void HAL_I2C_ER_IRQHandler(I2C_HandleTypeDef* /*hi2c*/)
{
    ++s_er_irq_calls;
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeI2C_Reset(void)
{
    s_i2c1 = I2C_TypeDef{};
    s_i2c2 = I2C_TypeDef{};
    s_i2c3 = I2C_TypeDef{};
    s_init_result     = HAL_OK;
    s_deinit_result   = HAL_OK;
    s_transfer_result = HAL_OK;
    s_state           = HAL_I2C_STATE_READY;
    s_abort_calls     = 0;
    s_ev_irq_calls    = 0;
    s_er_irq_calls    = 0;
}

extern "C" void FakeI2C_SetInitResult(HAL_StatusTypeDef result)     { s_init_result = result; }
extern "C" void FakeI2C_SetDeInitResult(HAL_StatusTypeDef result)   { s_deinit_result = result; }
extern "C" void FakeI2C_SetTransferResult(HAL_StatusTypeDef result) { s_transfer_result = result; }
extern "C" void FakeI2C_SetState(HAL_I2C_StateTypeDef state)        { s_state = state; }

extern "C" int FakeI2C_AbortCallCount(void)         { return s_abort_calls; }
extern "C" int FakeI2C_EvIRQHandlerCallCount(void)  { return s_ev_irq_calls; }
extern "C" int FakeI2C_ErIRQHandlerCallCount(void)  { return s_er_irq_calls; }
