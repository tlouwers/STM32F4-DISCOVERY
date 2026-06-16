#include "drivers/Button.hpp"

// User button: B1, PA0. Active high (external pull-down on the Discovery board).
static constexpr uint16_t BUTTON_PIN = GPIO_PIN_0;

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

bool Button::IsPressed()
{
    return (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOA, BUTTON_PIN));
}
