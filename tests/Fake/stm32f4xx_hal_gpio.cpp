/**
 * \file    stm32f4xx_hal_gpio.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL GPIO surface. Provides
 *          trivial HAL_GPIO_Init/WritePin/ReadPin/TogglePin bodies plus the EXTI
 *          IRQ-handler dispatch the Pin driver's ISR vectors route into. C
 *          linkage is preserved on every symbol the driver under test links
 *          against.
 *
 * \details HAL_GPIO_Init records the last-applied Mode/Pull and counts calls.
 *          HAL_GPIO_ReadPin returns a controllable pin state (RESET by default,
 *          which keeps I2C::RecoverBus's bit-bang loop deterministic, matching
 *          the old fake Pin's always-LOW Get()). HAL_GPIO_WritePin records the
 *          last state + count; HAL_GPIO_TogglePin counts. HAL_GPIO_EXTI_IRQHandler
 *          dispatches into the driver's HAL_GPIO_EXTI_Callback override so the
 *          per-line ISR -> registered pin callback path is reachable natively.
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
#include "stm32f4xx_hal_gpio.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

GPIO_PinState s_read_state      = GPIO_PIN_RESET;
GPIO_PinState s_last_write_state = GPIO_PIN_RESET;
int           s_init_calls       = 0;
int           s_write_calls      = 0;
int           s_toggle_calls      = 0;
uint32_t      s_last_init_mode    = 0;
uint32_t      s_last_init_pull    = 0;

} // namespace


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" void HAL_GPIO_Init(GPIO_TypeDef* /*GPIOx*/, GPIO_InitTypeDef* GPIO_Init)
{
    ++s_init_calls;
    if (GPIO_Init != nullptr)
    {
        s_last_init_mode = GPIO_Init->Mode;
        s_last_init_pull = GPIO_Init->Pull;
    }
}

extern "C" void HAL_GPIO_WritePin(GPIO_TypeDef* /*GPIOx*/, uint16_t /*GPIO_Pin*/, GPIO_PinState PinState)
{
    ++s_write_calls;
    s_last_write_state = PinState;
}

extern "C" GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* /*GPIOx*/, uint16_t /*GPIO_Pin*/)
{
    return s_read_state;
}

extern "C" void HAL_GPIO_TogglePin(GPIO_TypeDef* /*GPIOx*/, uint16_t /*GPIO_Pin*/)
{
    ++s_toggle_calls;
}

extern "C" void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)
{
    // On hardware this clears the EXTI pending bit and calls the callback;
    // model the dispatch unconditionally so the driver's per-line callback path
    // is reachable from a fired vector.
    HAL_GPIO_EXTI_Callback(GPIO_Pin);
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeGPIO_Reset(void)
{
    s_read_state       = GPIO_PIN_RESET;
    s_last_write_state = GPIO_PIN_RESET;
    s_init_calls       = 0;
    s_write_calls      = 0;
    s_toggle_calls     = 0;
    s_last_init_mode   = 0;
    s_last_init_pull   = 0;
}

extern "C" void          FakeGPIO_SetReadPinState(GPIO_PinState state) { s_read_state = state; }
extern "C" GPIO_PinState FakeGPIO_LastWriteState(void)                 { return s_last_write_state; }
extern "C" int           FakeGPIO_InitCallCount(void)                  { return s_init_calls; }
extern "C" int           FakeGPIO_WriteCallCount(void)                 { return s_write_calls; }
extern "C" int           FakeGPIO_ToggleCallCount(void)                { return s_toggle_calls; }
extern "C" uint32_t      FakeGPIO_LastInitMode(void)                   { return s_last_init_mode; }
extern "C" uint32_t      FakeGPIO_LastInitPull(void)                   { return s_last_init_pull; }
