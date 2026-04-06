#include "drivers/Led.hpp"

// Green LED: LD4, PD12
static constexpr uint16_t LED_PIN = GPIO_PIN_12;

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

void Led::Toggle()
{
    HAL_GPIO_TogglePin(GPIOD, LED_PIN);
}

void Led::On()
{
    HAL_GPIO_WritePin(GPIOD, LED_PIN, GPIO_PIN_SET);
}

void Led::Off()
{
    HAL_GPIO_WritePin(GPIOD, LED_PIN, GPIO_PIN_RESET);
}
