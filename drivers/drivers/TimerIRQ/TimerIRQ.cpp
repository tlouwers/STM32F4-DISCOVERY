/**
 * \file    TimerIRQ.cpp
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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/TimerIRQ
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/TimerIRQ/TimerIRQ.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static constexpr uint8_t SLOT_COUNT = static_cast<uint8_t>(TimerIRQ::Slot::DAC_UNDERRUN) + 1U;

static std::function<void()> sSlots[SLOT_COUNT];


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Invoke the handler registered for the given slot, if any.
 * \param   slot    The timer slot to dispatch.
 */
static void Dispatch(TimerIRQ::Slot slot)
{
    // Defence-in-depth: every caller passes a hardcoded enumerator, but guard
    // the array index anyway -- this runs in ISR context, so fail silently
    // rather than assert.
    const uint8_t index = static_cast<uint8_t>(slot);
    if (index >= SLOT_COUNT) { return; }

    const std::function<void()>& handler = sSlots[index];
    if (handler)
    {
        handler();
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
namespace TimerIRQ
{

/**
 * \brief   Register the handler invoked when the given timer's interrupt
 *          fires.
 * \param   slot    The timer slot to register.
 * \param   handler The handler to invoke from interrupt context.
 * \returns True if the slot was registered, else false.
 */
bool Install(Slot slot, const std::function<void()>& handler)
{
    EXPECT(static_cast<uint8_t>(slot) < SLOT_COUNT);
    if (static_cast<uint8_t>(slot) >= SLOT_COUNT) { return false; }

    sSlots[static_cast<uint8_t>(slot)] = handler;
    return true;
}

/**
 * \brief   Drop the handler for the given timer slot.
 * \param   slot    The timer slot to clear.
 * \returns True if the slot was cleared, else false.
 */
bool Uninstall(Slot slot)
{
    EXPECT(static_cast<uint8_t>(slot) < SLOT_COUNT);
    if (static_cast<uint8_t>(slot) >= SLOT_COUNT) { return false; }

    sSlots[static_cast<uint8_t>(slot)] = nullptr;
    return true;
}

} // namespace TimerIRQ


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
using TimerIRQ::Slot;

/**
 * \brief   ISR: TIM1 break + TIM9 global.
 */
extern "C" void TIM1_BRK_TIM9_IRQHandler(void)
{
    Dispatch(Slot::TIMER_1);
    Dispatch(Slot::TIMER_9);
}

/**
 * \brief   ISR: TIM1 update + TIM10 global.
 */
extern "C" void TIM1_UP_TIM10_IRQHandler(void)
{
    Dispatch(Slot::TIMER_1);
    Dispatch(Slot::TIMER_10);
}

/**
 * \brief   ISR: TIM1 trigger/commutation + TIM11 global.
 */
extern "C" void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
    Dispatch(Slot::TIMER_1);
    Dispatch(Slot::TIMER_11);
}

/**
 * \brief   ISR: TIM1 capture/compare.
 */
extern "C" void TIM1_CC_IRQHandler(void)
{
    Dispatch(Slot::TIMER_1);
}

/**
 * \brief   ISR: TIM2 global.
 */
extern "C" void TIM2_IRQHandler(void)
{
    Dispatch(Slot::TIMER_2);
}

/**
 * \brief   ISR: TIM3 global.
 */
extern "C" void TIM3_IRQHandler(void)
{
    Dispatch(Slot::TIMER_3);
}

/**
 * \brief   ISR: TIM4 global.
 */
extern "C" void TIM4_IRQHandler(void)
{
    Dispatch(Slot::TIMER_4);
}

/**
 * \brief   ISR: TIM5 global.
 */
extern "C" void TIM5_IRQHandler(void)
{
    Dispatch(Slot::TIMER_5);
}

/**
 * \brief   ISR: TIM6 global + DAC underrun (shared vector on the F407).
 */
extern "C" void TIM6_DAC_IRQHandler(void)
{
    Dispatch(Slot::TIMER_6);
    Dispatch(Slot::DAC_UNDERRUN);
}

/**
 * \brief   ISR: TIM7 global.
 */
extern "C" void TIM7_IRQHandler(void)
{
    Dispatch(Slot::TIMER_7);
}

/**
 * \brief   ISR: TIM8 break + TIM12 global.
 */
extern "C" void TIM8_BRK_TIM12_IRQHandler(void)
{
    Dispatch(Slot::TIMER_8);
    Dispatch(Slot::TIMER_12);
}

/**
 * \brief   ISR: TIM8 update + TIM13 global.
 */
extern "C" void TIM8_UP_TIM13_IRQHandler(void)
{
    Dispatch(Slot::TIMER_8);
    Dispatch(Slot::TIMER_13);
}

/**
 * \brief   ISR: TIM8 trigger/commutation + TIM14 global.
 */
extern "C" void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
    Dispatch(Slot::TIMER_8);
    Dispatch(Slot::TIMER_14);
}

/**
 * \brief   ISR: TIM8 capture/compare.
 */
extern "C" void TIM8_CC_IRQHandler(void)
{
    Dispatch(Slot::TIMER_8);
}
