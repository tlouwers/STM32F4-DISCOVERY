/**
 * \file    DAC.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Dac
 *
 * \brief   DAC peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/DAC
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

#ifndef DAC_HPP_
#define DAC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Dac
{
public:
    enum class Channel : uint8_t
    {
        CHANNEL_1,
        CHANNEL_2
    };

    /**
     * \enum    Precision
     * \brief   Available DAC precision (in bits).
     * \details Expressed as: Vout = (Vref x D) / 2^N
     */
    enum class Precision : uint8_t
    {
        _8_BIT_R,   ///< 2^8  =  256 steps (right aligned)
        _12_BIT_L,  ///< 2^12 = 4096 steps (left aligned)
        _12_BIT_R   ///< 2^12 = 4096 steps (right aligned), default
    };

    enum class Trigger : uint8_t
    {
        TIMER_2,
        TIMER_4,
        TIMER_5,
        TIMER_6,
        TIMER_7,
        TIMER_8,
        EXT_LINE_9,
        SOFTWARE    ///< Default
    };

    struct ChannelConfig
    {
        bool      started   = false;
        Precision precision = Precision::_12_BIT_R;
        Trigger   trigger   = Trigger::SOFTWARE;
    };


    Dac();
    virtual ~Dac();

    bool Init();
    bool IsInit() const;
    void Sleep();

    bool ConfigureChannel(const Channel& channel, const ChannelConfig& channelConfig);

    bool SetValue(const Channel& channel, uint16_t value);

private:
    DAC_HandleTypeDef mHandle = {};
    bool              mInitialized;
    ChannelConfig     mChannel1 = {};
    ChannelConfig     mChannel2 = {};

    void CheckAndEnableAHB1PeripheralClock();

    uint32_t GetTrigger(const Trigger& trigger);
    uint32_t GetAlignment(const Precision& precision);

    bool StartChannel(const Channel& channel);
    bool StopChannel(const Channel& channel);
};


#endif  // DAC_HPP_
