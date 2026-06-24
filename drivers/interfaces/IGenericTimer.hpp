/**
 * \file    IGenericTimer.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for GenericTimer driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IGENERICTIMER_HPP_
#define IGENERICTIMER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Interface for a general-purpose elapsed-timer driver.
 */
class IGenericTimer
{
public:
    virtual ~IGenericTimer() = default;

    /**
     * \brief   Start the timer and register the elapsed handler.
     * \param   handler     Callback invoked on each timer-elapsed event.
     * \returns True if the timer could be started, else false.
     * \note    The handler is invoked from INTERRUPT context
     *          (HAL_TIM_PeriodElapsedCallback). Keep it short, do not allocate
     *          heap, and use only the ...FromISR() variants of any RTOS APIs.
     */
    virtual bool Start(const std::function<void()>& handler) = 0;

    /**
     * \brief   Indicate whether the timer is running.
     * \returns True if started, else false.
     */
    virtual bool IsStarted() const = 0;

    /**
     * \brief   Stop the timer.
     * \returns True if stopped, else false.
     */
    virtual bool Stop() = 0;
};


#endif  // IGENERICTIMER_HPP_
