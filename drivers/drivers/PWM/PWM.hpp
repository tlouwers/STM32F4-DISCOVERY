/**
 * \file    PWM.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   PWM
 *
 * \brief   Helper class using Timer2..4 to provide PWM functionality.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/PWM
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

#ifndef PWM_HPP_
#define PWM_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IPWM.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    PwmTimerInstance
 * \brief   Available PWM timer instances.
 */
enum class PwmTimerInstance : uint8_t
{
    TIMER_2  =  2,
    TIMER_3  =  3,
    TIMER_4  =  4,
    TIMER_5  =  5
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class PWM final : public IPWM, public IConfigInitable
{
public:
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
    struct Config : IConfig
    {
        /**
         * \brief   Constructor of the PWM configuration struct.
         * \param   frequency   Clock frequency in Hz [0.1 .. 8000000.0].
         */
        explicit Config(float frequency) :
            mFrequency(frequency)
        { }

        float mFrequency;   ///< The frequency in Hz to use.
    };

    explicit PWM(const PwmTimerInstance& instance);
    virtual ~PWM();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool ConfigureChannel(const ChannelConfig& channelConfig);

    bool Start(Channel channel) override;
    bool Stop(Channel channel) override;

private:
    PwmTimerInstance  mInstance;
    TIM_HandleTypeDef mHandle = {};
    bool              mInitialized;

    void SetInstance(const PwmTimerInstance& instance);
    void CheckAndEnableAHB1PeripheralClock(const PwmTimerInstance& instance);
    void CheckAndDisbleAHB1PeripheralClock(const PwmTimerInstance& instance);
    uint16_t CalculatePeriod(float desiredFrequency);
    uint32_t CalculatePulse(uint8_t desiredDutyCycle, uint32_t period);
    uint32_t GetChannel(Channel channel);
    bool StopAllChannels();
};


#endif  // PWM_HPP_
