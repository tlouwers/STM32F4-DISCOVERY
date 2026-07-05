/**
 * \file    Led.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Led
 *
 * \brief   Green LED driver (LD4, PD12) for the blink_green sample application.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/blink_green/Src/drivers
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Led.hpp"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
// Green LED: LD4, PD12
static constexpr uint16_t LED_PIN = GPIO_PIN_12;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Configures PD12 as a push-pull output.
 */
void Led::Init()
{
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {};
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/**
 * \brief   Toggles the LED state.
 */
void Led::Toggle()
{
    HAL_GPIO_TogglePin(GPIOD, LED_PIN);
}

/**
 * \brief   Turns the LED on.
 */
void Led::On()
{
    HAL_GPIO_WritePin(GPIOD, LED_PIN, GPIO_PIN_SET);
}

/**
 * \brief   Turns the LED off.
 */
void Led::Off()
{
    HAL_GPIO_WritePin(GPIOD, LED_PIN, GPIO_PIN_RESET);
}
