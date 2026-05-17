/**
 * \file    BoardConfig.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Pin stubs for the shared root unit-test suite.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef BOARD_CONFIG_HPP_
#define BOARD_CONFIG_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Pin/Pin.hpp"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
// Motion (LIS3DSH) - SPI1
constexpr PinIdPort PIN_SPI1_CS     = { GPIO_PIN_3,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT1 = { GPIO_PIN_0,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT2 = { GPIO_PIN_1,  GPIOE };

// 8x8 LED Display - SPI2
constexpr PinIdPort PIN_SPI2_CS     = { GPIO_PIN_12, GPIOB };


#endif  // BOARD_CONFIG_HPP_
