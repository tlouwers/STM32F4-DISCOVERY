/**
 * \file    IPin.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for a GPIO pin.
 *
 * \details Abstracts the runtime operations of a pin (set/get/toggle and
 *          interrupt control) so consumers can be unit-tested against a
 *          mock. Pin construction and Configure() remain concrete on the
 *          Pin class: the composition root builds the real Pin, while
 *          tests inject a Mock_Pin through this interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef IPIN_HPP_
#define IPIN_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    Level
 * \brief   Pin level.
 */
enum class Level : bool
{
    LOW,
    HIGH
};

/**
 * \enum    Trigger
 * \brief   Interrupt trigger condition for a pin.
 */
enum class Trigger : uint8_t
{
    RISING,
    FALLING,
    BOTH
};


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
class IPin
{
public:
    virtual ~IPin() = default;

    /**
     * \brief   Configure (and optionally enable) an interrupt for the pin.
     * \param   trigger                 The edge condition on which to trigger.
     * \param   callback                The handler invoked when the interrupt fires.
     * \param   enabledAfterConfigure   If true, the interrupt is enabled once configured.
     * \returns True if the interrupt could be configured, else false.
     * \note    Handlers run in INTERRUPT context: keep them short, touch only
     *          ISR-safe shared state, and do no blocking work.
     */
    virtual bool Interrupt(Trigger trigger, const std::function<void()>& callback, bool enabledAfterConfigure) = 0;

    /**
     * \brief   Enable a previously configured interrupt for the pin.
     * \returns True if the interrupt could be enabled, else false.
     */
    virtual bool InterruptEnable() = 0;

    /**
     * \brief   Disable a previously configured interrupt for the pin.
     * \returns True if the interrupt could be disabled, else false.
     */
    virtual bool InterruptDisable() = 0;

    /**
     * \brief   Remove a previously configured interrupt for the pin.
     * \returns True if the interrupt could be removed, else false.
     */
    virtual bool InterruptRemove() = 0;

    /**
     * \brief   Toggle the output level of the pin (high to low, or low to high).
     */
    virtual void Toggle() const = 0;

    /**
     * \brief   Set the output level on the pin.
     * \param   level   The output level to set.
     */
    virtual void Set(Level level) = 0;

    /**
     * \brief   Get the actual level of the pin.
     * \returns The actual level of the pin.
     */
    virtual Level Get() const = 0;
};


#endif  // IPIN_HPP_
