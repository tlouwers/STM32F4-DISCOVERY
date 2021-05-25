/**
 * \file    BoardConfig.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   List all pins of the system.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2019
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

// Audio Amplifier - control (CS43L22) - I2C
constexpr PinIdPort PIN_I2C1_SCL    = { GPIO_PIN_6,  GPIOB };
constexpr PinIdPort PIN_I2C1_SDA    = { GPIO_PIN_9,  GPIOB };
constexpr PinIdPort PIN_AUDIO_nRST  = { GPIO_PIN_4,  GPIOD };

// Motion (LIS3DSH) - SPI1
constexpr PinIdPort PIN_SPI1_SCK    = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_SPI1_MISO   = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_SPI1_MOSI   = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_SPI1_CS     = { GPIO_PIN_3,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT1 = { GPIO_PIN_0,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT2 = { GPIO_PIN_1,  GPIOE };

// 8x8 LED Display - SPI2
constexpr PinIdPort PIN_SPI2_SCK    = { GPIO_PIN_10, GPIOB };
constexpr PinIdPort PIN_SPI2_MISO   = { GPIO_PIN_2,  GPIOC };   // Not used
constexpr PinIdPort PIN_SPI2_MOSI   = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_SPI2_CS     = { GPIO_PIN_2,  GPIOE };

// PWM
constexpr PinIdPort PIN_PWM_CH1     = { GPIO_PIN_15, GPIOA };

// DAC
constexpr PinIdPort PIN_DAC_CHANNEL1 = { GPIO_PIN_4,  GPIOA };
constexpr PinIdPort PIN_DAC_CHANNEL2 = { GPIO_PIN_5,  GPIOA };

// ADC
constexpr PinIdPort PIN_ADC1_CHANNEL11 = { GPIO_PIN_1,  GPIOC };
/*
constexpr PinIdPort PIN_ADC1_CHANNEL0  = { GPIO_PIN_0,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL1  = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL2  = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL3  = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL4  = { GPIO_PIN_4,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL5  = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL6  = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL7  = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL8  = { GPIO_PIN_0,  GPIOB };
constexpr PinIdPort PIN_ADC1_CHANNEL9  = { GPIO_PIN_1,  GPIOB };
constexpr PinIdPort PIN_ADC1_CHANNEL10 = { GPIO_PIN_0,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL11 = { GPIO_PIN_1,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL12 = { GPIO_PIN_2,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL13 = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL14 = { GPIO_PIN_4,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL15 = { GPIO_PIN_5,  GPIOC };

constexpr PinIdPort PIN_ADC2_CHANNEL0  = { GPIO_PIN_0,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL1  = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL2  = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL3  = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL4  = { GPIO_PIN_4,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL5  = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL6  = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL7  = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL8  = { GPIO_PIN_0,  GPIOB };
constexpr PinIdPort PIN_ADC2_CHANNEL9  = { GPIO_PIN_1,  GPIOB };
constexpr PinIdPort PIN_ADC2_CHANNEL10 = { GPIO_PIN_0,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL11 = { GPIO_PIN_1,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL12 = { GPIO_PIN_2,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL13 = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL14 = { GPIO_PIN_4,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL15 = { GPIO_PIN_5,  GPIOC };

constexpr PinIdPort PIN_ADC3_CHANNEL0  = { GPIO_PIN_0,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL1  = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL2  = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL3  = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL4  = { GPIO_PIN_6,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL5  = { GPIO_PIN_7,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL6  = { GPIO_PIN_8,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL8  = { GPIO_PIN_10, GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL9  = { GPIO_PIN_3,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL10 = { GPIO_PIN_0,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL11 = { GPIO_PIN_1,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL12 = { GPIO_PIN_2,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL13 = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL14 = { GPIO_PIN_4,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL15 = { GPIO_PIN_5,  GPIOF };
*/

#endif  // BOARD_CONFIG_HPP_
