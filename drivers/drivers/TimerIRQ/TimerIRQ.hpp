/**
 * \file    TimerIRQ.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Single owner of every timer interrupt vector on the STM32F407.
 *
 * \details The STM32F407 timer interrupt lines are shared: TIM1's
 *          BRK/UP/TRG_COM vectors are multiplexed with TIM9/10/11, TIM8's
 *          with TIM12/13/14, and TIM6's with the DAC underrun interrupt
 *          (the vector is TIM6_DAC_IRQHandler, there is no bare
 *          TIM6_IRQHandler on this part). If each timer driver defined its
 *          own extern "C" ISR symbol, two drivers landing on the same
 *          shared line (e.g. a future advanced-timer PWM on TIM1 alongside
 *          a GenericTimer on TIM9) would produce a duplicate-symbol link
 *          failure.
 *
 *          TimerIRQ owns the *only* copy of every timer ISR symbol and
 *          fans each vector out to a per-timer slot. Drivers register their
 *          HAL dispatch at Init() and unregister it at Sleep(); the
 *          user-facing driver API is unchanged.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/TimerIRQ
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef TIMER_IRQ_HPP_
#define TIMER_IRQ_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Namespace                                                            */
/************************************************************************/
namespace TimerIRQ
{

/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    Slot
 * \brief   One dispatch slot per timer (plus the DAC underrun line that
 *          shares TIM6's vector).
 * \note    Per-timer granularity: each shared vector calls every timer
 *          slot mapped to it, and the registered handler is expected to be
 *          HAL_TIM_IRQHandler(&handle), which already demuxes by the
 *          enabled+pending flag, so a no-op on the timer that did not fire
 *          is safe. TIMER_1/TIMER_8 slots are reserved for a future
 *          advanced-timer PWM and are simply unregistered (no-op) until then.
 * \note    Enumerators are TIMER_n / DAC_UNDERRUN, not TIMn / DAC: CMSIS
 *          stm32f407xx.h bare-defines TIM1..TIM14 and DAC as peripheral
 *          pointer macros, so an enumerator literally named TIM2 would
 *          preprocess into nonsense (same rule as the driver-class naming
 *          note in CLAUDE.md).
 */
enum class Slot : uint8_t
{
    TIMER_1,
    TIMER_2,
    TIMER_3,
    TIMER_4,
    TIMER_5,
    TIMER_6,
    TIMER_7,
    TIMER_8,
    TIMER_9,
    TIMER_10,
    TIMER_11,
    TIMER_12,
    TIMER_13,
    TIMER_14,
    DAC_UNDERRUN
};


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Register the handler invoked when the given timer's interrupt
 *          fires.
 * \param   slot    The timer slot to register.
 * \param   handler The handler to invoke from interrupt context (typically
 *                  HAL_TIM_IRQHandler(&handle)).
 * \returns True if the slot was registered, else false.
 * \note    Must be called with the corresponding NVIC line disabled (the
 *          driver Init() path guarantees this). std::function assignment is
 *          not atomic on Cortex-M4, so registering while the line is live
 *          would race the ISR.
 */
bool Install(Slot slot, const std::function<void()>& handler);

/**
 * \brief   Drop the handler for the given timer slot.
 * \param   slot    The timer slot to clear.
 * \returns True if the slot was cleared, else false.
 * \note    Must be called with the corresponding NVIC line disabled (the
 *          driver Sleep()/destructor path guarantees this) so a stale
 *          lambda capturing a destroyed object can never be dispatched.
 */
bool Uninstall(Slot slot);

} // namespace TimerIRQ


#endif  // TIMER_IRQ_HPP_
