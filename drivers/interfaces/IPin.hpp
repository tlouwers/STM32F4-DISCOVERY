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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
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

    virtual bool Interrupt(Trigger trigger, const std::function<void()>& callback, bool enabledAfterConfigure) = 0;
    virtual bool InterruptEnable() = 0;
    virtual bool InterruptDisable() = 0;
    virtual bool InterruptRemove() = 0;

    virtual void Toggle() const = 0;
    virtual void Set(Level level) = 0;
    virtual Level Get() const = 0;
};


#endif  // IPIN_HPP_
