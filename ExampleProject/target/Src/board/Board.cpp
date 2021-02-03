/**
 * \file Board.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Helper class intended to configure the pins and clock of the system.
 *
 *
 * \details Intended use it to have a single grouping of functionality which
 *          sets the clock and pins of the board into a defined state, and later
 *          into a defined sleep state (for low power behavior).
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        04-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "board/Board.hpp"
#include "board/BoardConfig.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Initialize the pins for the board.
 */
void Board::InitPins()
{
    // USART2
    Pin(PIN_USART2_RTS, Alternate::AF7);
    Pin(PIN_USART2_TX,  Alternate::AF7, PullUpDown::UP);
    Pin(PIN_USART2_RX,  Alternate::AF7, PullUpDown::UP);
    Pin(PIN_USART2_CTS, Alternate::AF7);

    // Audio Control - I2C
    Pin(PIN_I2C1_SCL, Alternate::AF4);
    Pin(PIN_I2C1_SDA, Alternate::AF4);

    // Motion - SPI1
    Pin(PIN_SPI1_SCK,  Alternate::AF5);
    Pin(PIN_SPI1_MISO, Alternate::AF5);
    Pin(PIN_SPI1_MOSI, Alternate::AF5);
    // PIN_SPI1_CS     -- ChipSelect, handled in software
    // PIN_MOTION_INT1 -- Input, handled in LIS3DSH class
    // PIN_MOTION_INT2 -- Input, handled in LIS3DSH class
}

/**
 * \brief   Initialize the clock(s) of the system.
 * \returns True if the cock(s) could be set successfully, else false.
 */
bool Board::InitClock()
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Enable Power Control clock
    __HAL_RCC_PWR_CLK_ENABLE();

    // The voltage scaling allows optimizing the power consumption when the
    // device is clocked below the maximum system frequency, to update the
    // voltage scaling value regarding system frequency refer to product
    // datasheet.
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Enable HSE Oscillator and activate PLL with HSE as source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return false;
    }

    // Select HSE as system clock source and configure the HCLK, PCLK1 and PCLK2
    // clocks dividers
    RCC_ClkInitStruct.ClockType = ( RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                    RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2 );
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        return false;
    }

    // Enables the Clock Security System
    HAL_RCC_EnableCSS();

    return true;
}

/**
 * \brief   Put all pins into a low-power state.
 */
void Board::Sleep()
{
    // ToDo

    Pin(PIN_USART2_RTS, PullUpDown::HIGHZ);
    Pin(PIN_USART2_TX,  PullUpDown::HIGHZ);
    Pin(PIN_USART2_RX,  PullUpDown::HIGHZ);
    Pin(PIN_USART2_CTS, PullUpDown::HIGHZ);
}

