/**
 * \file    HalTimer.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   HalTimer
 *
 * \brief   Helper class to provide timer tick to the STM32 HAL, based upon
 *          the GenericTimer class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/HalTimer
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    12-2021
 */

#ifndef HAL_TIMER_HPP_
#define HAL_TIMER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <functional>
#include "interfaces/IInitable.hpp"
#include "drivers/GenericTimer/GenericTimer.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class HalTimer final : public IConfig
{
public:
    /**
     * \struct  Config
     * \brief   Configuration struct for HalTimer.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the HalTimer configuration struct.
         * \param   frequency           Frequency of the timer in Hz. Range [1..10000] Hz.
         */
        Config(uint16_t frequency) :
            mFrequency(frequency)
        { }

        uint16_t mFrequency;    ///< Frequency in Hz.
    };

    explicit HalTimer(const GenericTimerInstance& instance);
    virtual ~HalTimer();

    bool Init(const IConfig& config);
    bool IsInit() const;

private:
    GenericTimer mGenericTimer;

    void SetInstance(const GenericTimerInstance& instance);
    void HalTimerTick();
};


#endif  // HAL_TIMER_HPP_
