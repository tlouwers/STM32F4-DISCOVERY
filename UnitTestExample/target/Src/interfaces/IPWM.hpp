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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
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
    /**
     * \enum    Channel
     * \brief   Available PWM channels.
     */
    enum class Channel : uint8_t
    {
        Channel_1 = 1,
        Channel_2,
        Channel_3,
        Channel_4
    };


    virtual bool Start(Channel channel) = 0;
    virtual bool Stop(Channel channel) = 0;
};


#endif  // IPWM_HPP_
