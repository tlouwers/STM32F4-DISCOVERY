/**
 * \file    Pin.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Pin
 *
 * \brief   Fake Pin driver implementation (no-ops) for FreeRTOSProject native
 *          unit tests, so non-injected components compile and link.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/FreeRTOSProject/tests/Fake/drivers/Pin
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "Pin.hpp"


Pin::Pin(PinIdPort idAndPort)
{}
Pin::Pin(PinIdPort idAndPort, Level level, Drive drive)
{}
Pin::Pin(PinIdPort idAndPort, PullUpDown pullUpDown)
{}
Pin::Pin(PinIdPort idAndPort, Alternate alternate, PullUpDown pullUpDown, Mode mode)
{}

void Pin::Configure(Level level, Drive drive)
{}
void Pin::Configure(PullUpDown pullUpDown)
{}
void Pin::Configure(Alternate alternate, PullUpDown pullUpDown, Mode mode)
{}

bool Pin::Interrupt(Trigger trigger, const std::function<void()>& callback, bool enabledAfterConfigure)
{
    return true;
}

bool Pin::InterruptEnable()
{
    return true;
}

bool Pin::InterruptDisable()
{
    return true;
}

bool Pin::InterruptRemove()
{
    return true;
}

void Pin::Toggle() const
{}
void Pin::Set(Level level)
{}

Level Pin::Get() const
{
    return Level::LOW;
}
