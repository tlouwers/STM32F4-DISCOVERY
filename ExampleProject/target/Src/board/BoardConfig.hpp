/**
 * \file BoardConfig.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   List all pins of the system.
 *
 * \details Intended use is to have a human readable list of all pins of the
 *          system.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        04-2019
 */

#ifndef BOARDCONFIG_HPP_
#define BOARDCONFIG_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Pin/Pin.hpp"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
// Button
constexpr PinIdPort PIN_BUTTON      = { GPIO_PIN_0,  GPIOA };

// Leds
constexpr PinIdPort PIN_LED_GREEN   = { GPIO_PIN_12, GPIOD };
constexpr PinIdPort PIN_LED_ORANGE  = { GPIO_PIN_13, GPIOD };
constexpr PinIdPort PIN_LED_RED     = { GPIO_PIN_14, GPIOD };
constexpr PinIdPort PIN_LED_BLUE    = { GPIO_PIN_15, GPIOD };

// Usart 2
constexpr PinIdPort PIN_USART2_RTS  = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_USART2_TX   = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_USART2_RX   = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_USART2_CTS  = { GPIO_PIN_3,  GPIOD };

// Audio Control (CS43L22) - I2C, address 0x94
constexpr PinIdPort PIN_I2C1_SCL    = { GPIO_PIN_6,  GPIOB };
constexpr PinIdPort PIN_I2C1_SDA    = { GPIO_PIN_9,  GPIOB };

// Motion (LIS3DSH) - SPI1
constexpr PinIdPort PIN_SPI1_SCK    = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_SPI1_MISO   = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_SPI1_MOSI   = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_SPI1_CS     = { GPIO_PIN_3,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT1 = { GPIO_PIN_0,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT2 = { GPIO_PIN_1,  GPIOE };


#endif  // BOARDCONFIG_HPP_
