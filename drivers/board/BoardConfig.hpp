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
 * \details Pin assignments follow UM1472 (STM32F407G-DISC1 user manual),
 *          Table 7 "STM32 pin description versus board functions".
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
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
// User button (B1) -- PA0-WKUP, active high
constexpr PinIdPort PIN_BUTTON          = { GPIO_PIN_0,  GPIOA };

// User LEDs -- active high
constexpr PinIdPort PIN_LED_GREEN       = { GPIO_PIN_12, GPIOD };    // LD4
constexpr PinIdPort PIN_LED_ORANGE      = { GPIO_PIN_13, GPIOD };    // LD3
constexpr PinIdPort PIN_LED_RED         = { GPIO_PIN_14, GPIOD };    // LD5
constexpr PinIdPort PIN_LED_BLUE        = { GPIO_PIN_15, GPIOD };    // LD6

// USART2 (AF7) -- available on extension header P1
constexpr PinIdPort PIN_USART2_RTS      = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_USART2_TX       = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_USART2_RX       = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_USART2_CTS      = { GPIO_PIN_3,  GPIOD };

// Audio DAC CS43L22 -- I2C1 control bus (AF4, open-drain)
constexpr PinIdPort PIN_I2C1_SCL        = { GPIO_PIN_6,  GPIOB };
constexpr PinIdPort PIN_I2C1_SDA        = { GPIO_PIN_9,  GPIOB };
constexpr PinIdPort PIN_AUDIO_nRST      = { GPIO_PIN_4,  GPIOD };    // CS43L22 active-low reset

// Audio DAC CS43L22 -- I2S3 audio data (AF6)
constexpr PinIdPort PIN_I2S3_WS         = { GPIO_PIN_4,  GPIOA };    // LRCK
constexpr PinIdPort PIN_I2S3_MCK        = { GPIO_PIN_7,  GPIOC };    // MCLK
constexpr PinIdPort PIN_I2S3_CK         = { GPIO_PIN_10, GPIOC };    // SCLK
constexpr PinIdPort PIN_I2S3_SD         = { GPIO_PIN_12, GPIOC };    // SDIN

// Digital microphone MP45DT02 -- I2S2 half-duplex (AF5)
constexpr PinIdPort PIN_I2S2_CK         = { GPIO_PIN_10, GPIOB };    // PDM clock out
constexpr PinIdPort PIN_I2S2_SD         = { GPIO_PIN_3,  GPIOC };    // PDM data in

// Motion sensor LIS3DSH -- SPI1 (AF5)
constexpr PinIdPort PIN_SPI1_SCK        = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_SPI1_MISO       = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_SPI1_MOSI       = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_SPI1_CS         = { GPIO_PIN_3,  GPIOE };    // Software-driven CS
constexpr PinIdPort PIN_MOTION_INT1     = { GPIO_PIN_0,  GPIOE };
constexpr PinIdPort PIN_MOTION_INT2     = { GPIO_PIN_1,  GPIOE };

// 8x8 LED matrix HI-M1388AR -- SPI2 (AF5)
constexpr PinIdPort PIN_SPI2_SCK        = { GPIO_PIN_13, GPIOB };
constexpr PinIdPort PIN_SPI2_MISO       = { GPIO_PIN_14, GPIOB };    // Not used
constexpr PinIdPort PIN_SPI2_MOSI       = { GPIO_PIN_15, GPIOB };
constexpr PinIdPort PIN_SPI2_CS         = { GPIO_PIN_12, GPIOB };    // Software-driven CS

// USB OTG FS -- micro-AB connector CN5 (AF10 for DM/DP/ID)
constexpr PinIdPort PIN_USB_OTG_FS_VBUS = { GPIO_PIN_9,  GPIOA };    // Sense input, no AF
constexpr PinIdPort PIN_USB_OTG_FS_ID   = { GPIO_PIN_10, GPIOA };
constexpr PinIdPort PIN_USB_OTG_FS_DM   = { GPIO_PIN_11, GPIOA };
constexpr PinIdPort PIN_USB_OTG_FS_DP   = { GPIO_PIN_12, GPIOA };
constexpr PinIdPort PIN_USB_PowerSwitchOn = { GPIO_PIN_0,  GPIOC };  // Enables STMPS2141 load switch
constexpr PinIdPort PIN_USB_OverCurrent = { GPIO_PIN_5,  GPIOD };    // Fault input from STMPS2141

// PWM
constexpr PinIdPort PIN_PWM_CH1         = { GPIO_PIN_15, GPIOA };

// DAC -- analog mode, no AF
constexpr PinIdPort PIN_DAC_CHANNEL1    = { GPIO_PIN_4,  GPIOA };
constexpr PinIdPort PIN_DAC_CHANNEL2    = { GPIO_PIN_5,  GPIOA };

// ADC1 -- only the channel wired to the extension header is listed;
// the full ADC1/2/3 channel-to-pin map is included for reference below.
constexpr PinIdPort PIN_ADC1_CHANNEL11  = { GPIO_PIN_1,  GPIOC };
/*
constexpr PinIdPort PIN_ADC1_CHANNEL0   = { GPIO_PIN_0,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL1   = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL2   = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL3   = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL4   = { GPIO_PIN_4,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL5   = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL6   = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL7   = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_ADC1_CHANNEL8   = { GPIO_PIN_0,  GPIOB };
constexpr PinIdPort PIN_ADC1_CHANNEL9   = { GPIO_PIN_1,  GPIOB };
constexpr PinIdPort PIN_ADC1_CHANNEL10  = { GPIO_PIN_0,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL11  = { GPIO_PIN_1,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL12  = { GPIO_PIN_2,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL13  = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL14  = { GPIO_PIN_4,  GPIOC };
constexpr PinIdPort PIN_ADC1_CHANNEL15  = { GPIO_PIN_5,  GPIOC };

constexpr PinIdPort PIN_ADC2_CHANNEL0   = { GPIO_PIN_0,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL1   = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL2   = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL3   = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL4   = { GPIO_PIN_4,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL5   = { GPIO_PIN_5,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL6   = { GPIO_PIN_6,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL7   = { GPIO_PIN_7,  GPIOA };
constexpr PinIdPort PIN_ADC2_CHANNEL8   = { GPIO_PIN_0,  GPIOB };
constexpr PinIdPort PIN_ADC2_CHANNEL9   = { GPIO_PIN_1,  GPIOB };
constexpr PinIdPort PIN_ADC2_CHANNEL10  = { GPIO_PIN_0,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL11  = { GPIO_PIN_1,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL12  = { GPIO_PIN_2,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL13  = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL14  = { GPIO_PIN_4,  GPIOC };
constexpr PinIdPort PIN_ADC2_CHANNEL15  = { GPIO_PIN_5,  GPIOC };

constexpr PinIdPort PIN_ADC3_CHANNEL0   = { GPIO_PIN_0,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL1   = { GPIO_PIN_1,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL2   = { GPIO_PIN_2,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL3   = { GPIO_PIN_3,  GPIOA };
constexpr PinIdPort PIN_ADC3_CHANNEL4   = { GPIO_PIN_6,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL5   = { GPIO_PIN_7,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL6   = { GPIO_PIN_8,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL8   = { GPIO_PIN_10, GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL9   = { GPIO_PIN_3,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL10  = { GPIO_PIN_0,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL11  = { GPIO_PIN_1,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL12  = { GPIO_PIN_2,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL13  = { GPIO_PIN_3,  GPIOC };
constexpr PinIdPort PIN_ADC3_CHANNEL14  = { GPIO_PIN_4,  GPIOF };
constexpr PinIdPort PIN_ADC3_CHANNEL15  = { GPIO_PIN_5,  GPIOF };
*/

#endif  // BOARD_CONFIG_HPP_
