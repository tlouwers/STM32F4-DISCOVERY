/**
 * \file    Button.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Button
 *
 * \brief   Onboard user button driver (B1, PA0) for the blink_green sample application.
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
#include "drivers/Button.hpp"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
// User button: B1, PA0. Active high (external pull-down on the Discovery board).
static constexpr uint16_t BUTTON_PIN = GPIO_PIN_0;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Configures PA0 as a floating input.
 */
void Button::Init()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {};
    GPIO_InitStruct.Pin   = BUTTON_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * \brief   Reads the current button state.
 * \returns True if the button is currently pressed.
 */
bool Button::IsPressed()
{
    return (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOA, BUTTON_PIN));
}
