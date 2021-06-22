/**
 * \file    IDAC.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for DAC peripheral driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IDAC_HPP_
#define IDAC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class IDAC
{
public:
    /**
     * \enum    Channel
     * \brief   Available DAC channels.
     */
    enum class Channel : uint8_t
    {
        CHANNEL_1,
        CHANNEL_2
    };


    virtual bool SetValue(const Channel& channel, uint16_t value) = 0;

    virtual bool StartWaveform(const Channel& channel) = 0;
    virtual bool StopWaveform(const Channel& channel) = 0;
};


#endif  // IDAC_HPP_
