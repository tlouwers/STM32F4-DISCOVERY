/**
 * \file    FakeHI_M1388AR.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   FakeHI_M1388AR
 *
 * \brief   Simulated driver for the HI-M1388AR 8x8 LED matrix display.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/components/HI-M1388AR
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef FAKE_HI_M1388AR_HPP_
#define FAKE_HI_M1388AR_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "Interfaces/IInitable.hpp"
#include "Interfaces/ISPI.hpp"
#include "drivers/Pin/Pin.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class FakeHI_M1388AR final : public IConfigInitable
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

    FakeHI_M1388AR(ISPI& spi, PinIdPort chipSelect) :
        mInitialized(false)
    {
        UNUSED(spi);
        UNUSED(chipSelect);
    }
    virtual ~FakeHI_M1388AR() {}

    bool Init(const IConfig& config) override { UNUSED(config); mInitialized = true; return true; }
    bool IsInit() const override { return mInitialized; }
    bool Sleep() override { mInitialized = false; return true; }

    bool ClearDisplay() { return ((mInitialized) ? true : false); }
    bool WriteDigits(const uint8_t* src)
    {
        if (src != nullptr)
        {
            return ((mInitialized) ? true : false);
        }
        return false;
    }

private:
    bool mInitialized;
};


#endif  // FAKE_HIM1388AR_HPP_
