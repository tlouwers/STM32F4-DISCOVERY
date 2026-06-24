/**
 * \file    IPWM.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for PWM peripheral driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IPWM_HPP_
#define IPWM_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class IPWM
{
public:
    virtual ~IPWM() = default;
    /**
     * \enum    Channel
     * \brief   Available PWM channels.
     */
    enum class Channel : uint8_t
    {
        CHANNEL_1 = 1,
        CHANNEL_2,
        CHANNEL_3,
        CHANNEL_4
    };


    /**
     * \brief   Start PWM output for the given channel.
     * \param   channel     The channel to start PWM output for.
     * \returns True if PWM output could be started for the channel, else false.
     */
    virtual bool Start(Channel channel) = 0;

    /**
     * \brief   Stop PWM output for the given channel.
     * \param   channel     The channel to stop PWM output for.
     * \returns True if PWM output could be stopped for the channel, else false.
     */
    virtual bool Stop(Channel channel) = 0;
};


#endif  // IPWM_HPP_
