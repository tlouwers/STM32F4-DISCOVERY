/**
 * \file    IDelay.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for a blocking millisecond delay.
 *
 * \details Abstracts a blocking wait so application logic can depend on this
 *          contract instead of calling HAL_Delay() directly. The composition
 *          root injects the concrete HAL-backed delay; unit tests inject a
 *          recording mock that captures the requested duration and returns
 *          immediately, so a long firmware wait never costs real wall-clock
 *          time in the test suite.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef IDELAY_HPP_
#define IDELAY_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class IDelay
{
public:
    virtual ~IDelay() = default;

    /**
     * \brief   Block for the given number of milliseconds.
     * \param   ms  Duration to wait, in milliseconds.
     */
    virtual void DelayMs(uint32_t ms) = 0;
};


#endif  // IDELAY_HPP_
