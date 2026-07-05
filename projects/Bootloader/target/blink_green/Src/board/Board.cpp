/**
 * \file    Board.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Board
 *
 * \brief   Clock configuration for the blink_green sample application.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/blink_green/Src/board
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "board/Board.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Configures SYSCLK to run directly off the 8 MHz HSE oscillator
 *          (no PLL), with all bus prescalers at /1 and zero flash wait states.
 * \returns True if the oscillator and clock switch both configured
 *          successfully, false otherwise.
 */
bool Board::InitClock()
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {};

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.LSIState       = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 4;
    RCC_OscInitStruct.PLL.PLLN       = 72;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ       = 3;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return false;
    }

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
    RCC_ClkInitStruct.ClockType      = ( RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2 );
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        return false;
    }

    HAL_RCC_EnableCSS();
    return true;
}
