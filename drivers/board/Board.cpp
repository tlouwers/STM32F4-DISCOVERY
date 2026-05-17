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
 * \brief   Helper class intended to configure the pins and clock of the system.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    04-2019
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
 * \note    Pins owned by a specific driver (chip-selects, interrupts, LEDs,
 *          button, DAC/ADC analog inputs) are configured by that driver.
 *          Only shared/alternate-function pins are set up here.
 */
void Board::InitPins()
{
    // USART2 (AF7)
    Pin(PIN_USART2_RTS, Alternate::AF7);
    Pin(PIN_USART2_TX,  Alternate::AF7, PullUpDown::UP);
    Pin(PIN_USART2_RX,  Alternate::AF7, PullUpDown::UP);
    Pin(PIN_USART2_CTS, Alternate::AF7);

    // Audio Control -- I2C1 (AF4, open-drain; board has external pull-ups)
    Pin(PIN_I2C1_SCL, Alternate::AF4, PullUpDown::HIGHZ, Mode::OPEN_DRAIN);
    Pin(PIN_I2C1_SDA, Alternate::AF4, PullUpDown::HIGHZ, Mode::OPEN_DRAIN);

    // Motion -- SPI1 (AF5)
    Pin(PIN_SPI1_SCK,  Alternate::AF5);
    Pin(PIN_SPI1_MISO, Alternate::AF5);
    Pin(PIN_SPI1_MOSI, Alternate::AF5);
    // PIN_SPI1_CS     -- ChipSelect, handled in software
    // PIN_MOTION_INT1 -- Input, handled in LIS3DSH class
    // PIN_MOTION_INT2 -- Input, handled in LIS3DSH class

    // HI-M1388AR -- SPI2 (AF5)
    Pin(PIN_SPI2_SCK,  Alternate::AF5);
    // PIN_SPI2_MISO   -- MISO, not used
    Pin(PIN_SPI2_MOSI, Alternate::AF5);
    // PIN_SPI2_CS     -- ChipSelect, handled in software

    // Audio DAC CS43L22 -- I2S3 data (AF6). Uncomment when enabling audio
    // output; requires PLLI2S to be running (see InitClock).
    // Pin(PIN_I2S3_WS,  Alternate::AF6);
    // Pin(PIN_I2S3_MCK, Alternate::AF6);
    // Pin(PIN_I2S3_CK,  Alternate::AF6);
    // Pin(PIN_I2S3_SD,  Alternate::AF6);
    // PIN_AUDIO_nRST  -- GPIO output, handled in CS43L22 driver

    // Microphone MP45DT02 -- I2S2 (AF5). Uncomment when enabling the mic;
    // requires PLLI2S to be running (see InitClock).
    // Pin(PIN_I2S2_CK, Alternate::AF5);
    // Pin(PIN_I2S2_SD, Alternate::AF5);

    // USB OTG FS -- CN5 (AF10). Uncomment when enabling USB; requires 48 MHz
    // on PLL48CK (only available when the main PLL is used, see InitClock).
    // Pin(PIN_USB_OTG_FS_ID, Alternate::AF10, PullUpDown::UP);
    // Pin(PIN_USB_OTG_FS_DM, Alternate::AF10);
    // Pin(PIN_USB_OTG_FS_DP, Alternate::AF10);
    // PIN_USB_OTG_FS_VBUS     -- Input, no AF; handled in USB stack
    // PIN_USB_PowerSwitchOn   -- GPIO output, handled in USB stack
    // PIN_USB_OverCurrent     -- Input, handled in USB stack
}

/**
 * \brief   Initialize the clock(s) of the system.
 * \returns True if the clock(s) could be set successfully, else false.
 * \note    The clock profile is selected at compile time via BOARD_USE_PLL
 *          (and BOARD_USE_PLLI2S for audio) in BoardConfig.hpp. The default
 *          (BOARD_USE_PLL=0) runs SYSCLK direct from the 8 MHz HSE -- lowest
 *          power but no USB / I2S / RNG. Set BOARD_USE_PLL=1 to run at the
 *          168 MHz board maximum with PLL48CK = 48 MHz available.
 */
bool Board::InitClock()
{
    // Enable Power Control clock (ES0182 2.2.13: HAL macro handles the
    // post-enable dummy-read delay, so no explicit barrier is needed).
    __HAL_RCC_PWR_CLK_ENABLE();

    // Required for RTC / backup register access.
    HAL_PWR_EnableBkUpAccess();

    // VOS = Scale 1 is the default after reset and is compatible with any
    // SYSCLK up to 168 MHz. Keep it explicit for clarity.
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Enable HSE (8 MHz, bypass from ST-LINK MCO) and LSI (for IWDG / RTC).
    RCC_OscInitTypeDef RCC_OscInitStruct = {};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.LSIState       = RCC_LSI_ON;

#if BOARD_USE_PLL
    // HSE = 8 MHz
    //   PLLM = 8    -> VCO input = 1 MHz    (RM0090 recommends 2 MHz, 1 MHz works)
    //   PLLN = 336  -> VCO       = 336 MHz  (allowed range 100..432 MHz)
    //   PLLP = 2    -> SYSCLK    = 168 MHz  (board maximum)
    //   PLLQ = 7    -> PLL48CK   = 48 MHz   (exact, required for USB OTG FS)
    RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM      = 8;
    RCC_OscInitStruct.PLL.PLLN      = 336;
    RCC_OscInitStruct.PLL.PLLP      = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ      = 7;
#else
    RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_OFF;
#endif

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        return false;
    }

#if BOARD_USE_PLLI2S
    // PLLI2SN = 258, PLLI2SR = 3 -> I2SCLK = (1 MHz * 258) / 3 = 86 MHz,
    // suitable for accurate 8 / 16 / 32 / 48 kHz audio sample rates.
    RCC_PeriphCLKInitTypeDef periph = {};
    periph.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    periph.PLLI2S.PLLI2SN       = 258;
    periph.PLLI2S.PLLI2SR       = 3;
    if (HAL_RCCEx_PeriphCLKConfig(&periph) != HAL_OK)
    {
        return false;
    }
#endif

    RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                       RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
#if BOARD_USE_PLL
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;       // 168 / 4 = 42 MHz (APB1 max)
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;       // 168 / 2 = 84 MHz (APB2 max)
    const uint32_t flashLatency      = FLASH_LATENCY_5;     // required for 168 MHz at VOS1
#else
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;       // 8 MHz, well under 42 MHz max
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;       // 8 MHz, well under 84 MHz max
    const uint32_t flashLatency      = FLASH_LATENCY_0;     // sufficient for SYSCLK <= 30 MHz at VOS1
#endif

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flashLatency) != HAL_OK)
    {
        return false;
    }

    // Enable the Clock Security System: triggers NMI if HSE fails.
    HAL_RCC_EnableCSS();

    return true;
}

/**
 * \brief   Put all mapped pins into a low-leakage state.
 * \details Follows AN4899: unused GPIOs are cheapest as analog-input with
 *          no pull. AF pins (communication busses) are returned to HIGHZ
 *          input to avoid contention with external devices that may still
 *          be driving. Drivers owning a peripheral (LIS3DSH, CS43L22,
 *          HI-M1388AR) are responsible for putting their device into
 *          low-power mode before Sleep() is called.
 */
void Board::Sleep()
{
    // Communication busses -- release to HIGHZ so external masters/slaves
    // are not back-powered through the pins.
    Pin(PIN_USART2_RTS, PullUpDown::HIGHZ);
    Pin(PIN_USART2_TX,  PullUpDown::HIGHZ);
    Pin(PIN_USART2_RX,  PullUpDown::HIGHZ);
    Pin(PIN_USART2_CTS, PullUpDown::HIGHZ);

    Pin(PIN_I2C1_SCL,   PullUpDown::HIGHZ);
    Pin(PIN_I2C1_SDA,   PullUpDown::HIGHZ);

    Pin(PIN_SPI1_SCK,   PullUpDown::HIGHZ);
    Pin(PIN_SPI1_MISO,  PullUpDown::HIGHZ);
    Pin(PIN_SPI1_MOSI,  PullUpDown::HIGHZ);

    Pin(PIN_SPI2_SCK,   PullUpDown::HIGHZ);
    Pin(PIN_SPI2_MOSI,  PullUpDown::HIGHZ);

    // Outputs driven by the MCU -- park at inactive level.
    Pin(PIN_LED_GREEN,  Level::LOW);
    Pin(PIN_LED_ORANGE, Level::LOW);
    Pin(PIN_LED_RED,    Level::LOW);
    Pin(PIN_LED_BLUE,   Level::LOW);

    Pin(PIN_SPI1_CS,    Level::HIGH);   // CS idle-high
    Pin(PIN_SPI2_CS,    Level::HIGH);   // CS idle-high
    Pin(PIN_AUDIO_nRST, Level::LOW);    // Hold CS43L22 in reset

    // User button stays HIGHZ so WKUP still works from stop mode.
    Pin(PIN_BUTTON,     PullUpDown::HIGHZ);

    // Motion interrupt inputs: analog stops Schmitt-trigger current draw.
    Pin(PIN_MOTION_INT1, PullUpDown::ANALOG);
    Pin(PIN_MOTION_INT2, PullUpDown::ANALOG);
}
