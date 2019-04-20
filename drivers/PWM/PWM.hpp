/*
 * PWM.hpp
 *
 *  Created on: 19 apr. 2019
 *      Author: Terry
 */

#ifndef PWM_HPP_
#define PWM_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "stm32f4xx_hal.h"


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
         * \param   dutyCycle   Percentage, on time [1..100].
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
        Config(uint16_t frequency) :
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
    void CheckAndEnableAHB1PeripheralClock();
    void DisableAHB1PeripheralClock();
    uint32_t GetChannel(Channel channel);
    bool StopAllChannels();
};


#endif  // PWM_HPP_
