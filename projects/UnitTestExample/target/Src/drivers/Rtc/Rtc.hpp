/**
 * \file    Rtc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Rtc
 *
 * \brief   Rtc peripheral driver class.
 *
 * \note    This assumes the RTC is configured and running, using the LSE (slow
 *          crystal, 32768 Hz). Most likely this is done in Board::InitClock().
 *          Alternative is to use the HSE clock or the LSI clock (default), but
 *          these are less accurate.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/Rtc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef RTC_HPP_
#define RTC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IRTC.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Rtc final : public IRtc, public IConfigInitable
{
public:
    /**
     * \enum    ClockSource
     * \brief   Available RTC clock sources.
     */
    enum class ClockSource : uint8_t
    {
        LSI,    ///< Low Speed Internal. Default
        LSE,    ///< Low Speed External. 32 kHz crystal
        HSE,    ///< High Speed External. 8 MHz crystal
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for RTC.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the RTC configuration struct.
         * \param   clockSource     The clock source of the RTC to use.
         */
        explicit Config(ClockSource clockSource) :
            mClockSource(clockSource)
        { }

        ClockSource mClockSource;   ///< The clock source of the RTC to use.
    };

    Rtc();
    virtual ~Rtc();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool SetDateTime(const DateTime& dateTime) override;
    bool GetDateTime(DateTime& dateTime) override;

private:
    RTC_HandleTypeDef mHandle = {};
    bool              mInitialized;

    void EnablePeripheralClock(const ClockSource& clockSource);
    void DisablePeripheralClock();
};


#endif  // RTC_HPP_
