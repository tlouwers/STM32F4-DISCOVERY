/**
 * \file    HI-M1388AR.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   HI_M1388AR
 *
 * \brief   Driver for the HI_M1388AR 8x8 LED matrix display.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/components/HI-M1388AR
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "components/HI-M1388AR/HI-M1388AR.hpp"
#include "utility/SlimAssert/SlimAssert.h"


/************************************************************************/
/* Register Map                                                         */
/************************************************************************/
//static constexpr uint8_t NO_OP        = 0x00;
static constexpr uint8_t DIGIT_0      = 0x01;
static constexpr uint8_t DIGIT_1      = 0x02;
static constexpr uint8_t DIGIT_2      = 0x03;
static constexpr uint8_t DIGIT_3      = 0x04;
static constexpr uint8_t DIGIT_4      = 0x05;
static constexpr uint8_t DIGIT_5      = 0x06;
static constexpr uint8_t DIGIT_6      = 0x07;
static constexpr uint8_t DIGIT_7      = 0x08;
static constexpr uint8_t DECODE_MODE  = 0x09;
static constexpr uint8_t INTENSITY    = 0x0A;
static constexpr uint8_t SCAN_LIMIT   = 0x0B;
static constexpr uint8_t SHUTDOWN     = 0x0C;
//static constexpr uint8_t DISPLAY_TEST = 0x0F;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, configures ChipSelect pin.
 * \param   spi         SPI peripheral driver class.
 * \param   chipSelect  Pin ChipSelect, needed for SPI communication, toggled
 *                      within this class.
 */
HI_M1388AR::HI_M1388AR(SPI& spi, PinIdPort chipSelect) :
    mSpi(spi),
    mChipSelect(chipSelect, Level::HIGH)
{ }

/**
 * \brief   Destructor, configures pins to HIGHZ.
 */
HI_M1388AR::~HI_M1388AR()
{
    Sleep();
}

/**
 * \brief   Initializes the HI-M1388AR module.
 * \param   config  Configuration struct for HI-M1388AR module.
 * \returns True if the HI-M1388AR module could be initialized, else false.
 */
bool HI_M1388AR::Init(const Config& config)
{
    mChipSelect.Configure(Level::HIGH);

    bool result = Configure(config);
    ASSERT(result);

    if (result)
    {
        mInitialized = true;
    }

    return result;
}

/**
 * \brief   Indicate if HI-M1388AR is initialized.
 * \returns True if HI-M1388AR is initialized, else false.
 */
bool HI_M1388AR::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the HI-M1388AR module in sleep mode.
 * \details Configures CS pin to HIGHZ.
 * \returns True if HI-M1388AR module could be put in sleep mode, else false.
 */
bool HI_M1388AR::Sleep()
{
    bool result = ClearDisplay();
    ASSERT(result);

    result &= WriteRegister(SHUTDOWN, 0x00);
    ASSERT(result);

    mChipSelect.Configure(PullUpDown::HIGHZ);

    mInitialized = false;

    return result;
}

/**
 * \brief   Clear the display by writing all 0's to it.
 * \returns True if display could be cleared, else false.
 */
bool HI_M1388AR::ClearDisplay()
{
    bool result = false;

    if (mInitialized)
    {
        // All leds off
        uint8_t buffer[8] = {};
        result = WriteDigits(buffer);
    }

    return result;
}

/**
 * \brief   Write all 8 lines of the 8x8 matrix to the display.
 * \details Buffer of length 8, has 1 line per byte.
 * \param   src     Pointer to 8 byte long buffer with digit values.
 * \returns True if lines could be written, else false.
 */
bool HI_M1388AR::WriteDigits(const uint8_t* src)
{
    ASSERT(src);

    bool result = false;

    if (mInitialized)
    {
        result = WriteRegister(DIGIT_0, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_1, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_2, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_3, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_4, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_5, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_6, *src++);
        ASSERT(result);
        result &= WriteRegister(DIGIT_7, *src);
        ASSERT(result);
    }

    return result;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Configures the 8x8 LED matrix display for use: all leds of and
 *          intensity configured.
 * \param   config  Configuration struct for HI-M1388AR module.
 * \returns True if the HI-M1388AR module could be configured, else false.
 */
bool HI_M1388AR::Configure(const Config& config)
{
    if (config.mBrightness > 0x0F) { return false; }

    // Enable
    bool result = WriteRegister(SHUTDOWN, 0x01);

    // Intensity to 0x00
    result &= WriteRegister(INTENSITY, 0x00);

    // Decode mode to 0x00
    result &= WriteRegister(DECODE_MODE, 0x00);

    // Scan limit to 0x07 (all digits)
    result &= WriteRegister(SCAN_LIMIT, 0x07);

    // Initial value = all leds off
    result &= ClearDisplay();

    // Intensity to set value
    result &= WriteRegister(INTENSITY, config.mBrightness);

    return result;
}

/**
 * \brief   Write value to register using blocking SPI call.
 * \param   reg     The register to write to.
 * \param   value   The value to write to the register.
 * \returns True if the value could be written to the register, else false.
 */
bool HI_M1388AR::WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2] = { reg, value };

    mChipSelect.Set(Level::LOW);
    bool result = mSpi.WriteBlocking(buffer, 2);
    ASSERT(result);
    mChipSelect.Set(Level::HIGH);

    return result;
}
