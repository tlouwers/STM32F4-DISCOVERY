/**
 * \file    IBasicTimer.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for BasicTimer driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IBASICTIMER_HPP_
#define IBASICTIMER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Interface for a basic hardware timer driver (counter enable/disable).
 */
class IBasicTimer
{
public:
    virtual ~IBasicTimer() = default;

    /**
     * \brief   Enables the timer counter.
     * \note    This interface deliberately exposes no callback-registration
     *          API. BasicTimer's only documented consumer is the DAC TRGO
     *          chain (CPU-less); the concrete driver installs a TimerIRQ
     *          slot but does NOT enable the update-event interrupt, so no
     *          periodic CPU callback is required. To activate a periodic
     *          CPU callback in the future, add
     *          `RegisterCallback(std::function<void()>)` here and switch
     *          the concrete `Start()` to `HAL_TIM_Base_Start_IT`. See
     *          driver_update.md "BasicTimer ### 1" and ledger L5b for the
     *          deferred-decision rationale.
     * \returns True if started, else false.
     */
    virtual bool Start() = 0;

    /**
     * \brief   Indicate whether the timer counter is running.
     * \returns True if started, else false.
     */
    virtual bool IsStarted() const = 0;

    /**
     * \brief   Disable the timer counter.
     * \returns True if stopped, else false.
     */
    virtual bool Stop() = 0;
};


#endif  // IBASICTIMER_HPP_
