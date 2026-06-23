/**
 * \file    IDac.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for Dac peripheral driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
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
class IDac
{
public:
    virtual ~IDac() = default;
    /**
     * \enum    Channel
     * \brief   Available Dac channels.
     */
    enum class Channel : uint8_t
    {
        CHANNEL_1,
        CHANNEL_2
    };


    /**
     * \brief   Output a single value on the given DAC channel.
     * \param   channel     The channel on which to output the value.
     * \param   value       The value to output, in DAC counts.
     * \returns True if the value could be output, else false.
     */
    virtual bool SetValue(const Channel& channel, uint16_t value) = 0;

    /**
     * \brief   Start the configured (DMA) waveform on the given channel.
     * \param   channel     The channel to output the waveform on.
     * \returns True if the waveform could be started, else false.
     * \note    The waveform buffer is owned by the DMA controller from the moment
     *          this call succeeds until StopWaveform() completes. Do NOT modify or
     *          free it while the waveform is running -- a CPU write concurrent with
     *          the DMA read is a data race.
     */
    virtual bool StartWaveform(const Channel& channel) = 0;

    /**
     * \brief   Stop the running waveform on the given channel.
     * \param   channel     The channel whose waveform to stop.
     * \returns True if the waveform could be stopped, else false.
     */
    virtual bool StopWaveform(const Channel& channel) = 0;
};


#endif  // IDAC_HPP_
