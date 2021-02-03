/**
 * \file PWM.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   PWM class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/PWM
 *
 * \details Intended use is to provide an easier means to work with PWM
 *          channels. For this driver it is hardcoded to timer 2, all 4 channels
 *          can be used.
 *          Can be ported easily to timer 3, 4 or 5 as well, since most features
 *          are generic. This is something for a future update.
 *          Hardcoded items:
 *          Using timer 2, which is using APB1 timer clock as clock source.
 *
 * \note    Inspiration from:
 *          https://stm32f4-discovery.net/2014/05/stm32f4-stm32f429-discovery-pwm-tutorial/
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        04-2019
 */

#ifndef PWM_HPP_
#define PWM_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"


// For Timer 2 (General purpose) on STM32F407G

/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   PWM driver class.
 */
class PWM
{
public:
    /**
     * \enum    Channel
     * \brief   Available PWM channels.
     */
    enum class Channel : uint8_t
    {
        Channel_1 = 1,
        Channel_2,
        Channel_3,
        Channel_4,
    };

    /**
     * \enum    Polarity
     * \brief   Polarity of the ON time of the PWM signal.
     */
    enum class Polarity : bool
    {
        Low = 0,
        High
    };

    /**
     * \struct  ChannelConfig
     * \brief   Configuration struct for a channel of PWM.
     */
    struct ChannelConfig
    {
        /**
         * \brief   Constructor of the PWM channel configuration struct.
         * \param   channel     The PWM channel to use.
         * \param   dutyCycle   Percentage, on time [0..100].
         * \param   polarity    Polarity of the on time, high (default) or low.
         */
        ChannelConfig(Channel channel, uint8_t dutyCycle, Polarity polarity = Polarity::High) :
            mChannel(channel),
            mDutyCycle(dutyCycle),
            mPolarity(polarity)
        { }

        Channel  mChannel;      ///< The PWM channel to use.
        uint8_t  mDutyCycle;    ///< The duty cycle to use, as percentage.
        Polarity mPolarity;     ///< Polarity of the on time.
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for PWM.
     */
    struct Config
    {
        /**
         * \brief   Constructor of the PWM configuration struct.
         * \param   frequency   Clock frequency in Hz [1..65535].
         */
        explicit Config(uint16_t frequency) :
            mFrequency(frequency)
        { }

        uint16_t mFrequency;    ///< The frequency in Hz to use.
    };

    PWM();
    virtual ~PWM();

    bool Init(const Config& config);
    bool Sleep();

    bool ConfigureChannel(const ChannelConfig& channelConfig);

    bool Start(Channel channel);
    bool Stop(Channel channel);

private:
    TIM_HandleTypeDef mHandle = {};
    bool mInitialized;
    void CheckAndEnableAPB1TimerClock();
    void DisableAPB1TimerClock();
    uint32_t GetChannel(Channel channel);
    bool StopAllChannels();
};


#endif  // PWM_HPP_
