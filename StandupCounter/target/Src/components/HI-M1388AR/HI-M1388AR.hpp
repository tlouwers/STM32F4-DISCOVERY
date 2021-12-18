/**
 * \file    HI-M1388AR.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   HI_M1388AR
 *
 * \brief   Driver for the HI-M1388AR 8x8 LED matrix display.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/components/HI-M1388AR
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

#ifndef HI_M1388AR_HPP_
#define HI_M1388AR_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/ISPI.hpp"
#include "drivers/Pin/Pin.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class HI_M1388AR final : public IConfigInitable
{
public:
    /**
     * \struct  Config
     * \brief   Configuration struct for HI_M1388AR.
     */
    struct Config : IConfig
    {
        /**
         * \brief   Constructor of the HI_M1388AR configuration struct.
         * \param   brightness  Brightness of the LED display [0..15]. Default 8.
         */
        explicit Config(uint8_t brightness = 8) :
            mBrightness(brightness)
        { }

        uint8_t mBrightness;    ///< Brightness of the LED display.
    };

    HI_M1388AR(ISPI& spi, PinIdPort chipSelect);
    virtual ~HI_M1388AR();

    bool Init(const IConfig& config);
    bool IsInit() const;
    bool Sleep();

    bool ClearDisplay();
    bool WriteDigits(const uint8_t* src);

private:
    ISPI& mSpi;
    Pin   mChipSelect;
    bool  mInitialized;

    bool Configure(const IConfig& config);

    bool WriteRegister(uint8_t reg, uint8_t value);
};


#endif  // HI_M1388AR_HPP_
