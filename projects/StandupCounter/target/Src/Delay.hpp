/**
 * \file     Delay.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Delay
 *
 * \brief   HAL-backed blocking delay for StandupCounter.
 *
 * \details Concrete IDelay implementation that forwards to HAL_Delay(). The
 *          composition root constructs one and injects it into AppLogic, which
 *          depends only on the IDelay contract so it can be unit-tested with a
 *          recording mock instead of waiting real time.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/StandupCounter/target/Src
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef DELAY_HPP_
#define DELAY_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "interfaces/IDelay.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Delay final : public IDelay
{
public:
    Delay() = default;
    virtual ~Delay() = default;

    void DelayMs(uint32_t ms) override;
};


#endif  // DELAY_HPP_
