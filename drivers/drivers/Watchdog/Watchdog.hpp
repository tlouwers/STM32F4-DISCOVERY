/**
 * \file    Watchdog.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Watchdog
 *
 * \brief   Watchdog (IWDG) peripheral driver class.
 *
 * \note    The IWDG depends on the LSI clock to be available and running.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/Watchdog
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2022
 */

#ifndef WATCHDOG_HPP_
#define WATCHDOG_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "interfaces/IInitable.hpp"
#include "interfaces/IWatchdog.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Watchdog final : public IWatchdog, public IConfigInitable
{
public:
    /**
     * \enum    Timeout
     * \brief   Available timeout values.
     */
    enum class Timeout : uint8_t
    {
        _5_MS,
        _10_MS,
        _25_MS,
        _50_MS,
        _125_MS,
        _250_MS,
        _500_MS,
        _1_S,
        _2_S,
        _4_S,
        _8_S,
        _16_S,
        _32_S
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for Watchdog.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the Watchdog configuration struct.
         * \param   timeout     Timeout value.
         */
        Config(Timeout timeout = Timeout::_4_S) :
            mTimeout(timeout)
        { }

        Timeout mTimeout;  ///< Timeout value.
    };

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    void Refresh() const override;

private:
    IWDG_HandleTypeDef mHandle = {};
    bool               mInitialized;

    bool IsLSIClockEnabled() const;
    uint32_t CalculatePrescaler(Timeout timeout);
    uint32_t CalculateReload(Timeout timeout);
};

#endif  // WATCHDOG_HPP_
