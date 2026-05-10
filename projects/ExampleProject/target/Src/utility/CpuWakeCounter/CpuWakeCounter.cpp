/**
 * \file    CpuWakeCounter.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   CpuWakeCounter
 *
 * \brief   Helper class to measure CPU wake percentage.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/utility/CpuWakeCounter
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.2
 * \date    01-2022
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "CpuWakeCounter.hpp"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Initialize the DWT (Data Watchpoint and Trace) unit on the
 *          microcontroller and start a fresh measurement window.
 * \details Re-callable: a second Init() resets all per-window state so the
 *          first published statistics after re-Init reflect only post-Init
 *          activity.
 * \returns True if the DWT could be initialized, else false.
 */
bool CpuWakeCounter::Init()
{
    // Enable TRC
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;    // ~0x01000000 -- Disable TRC
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;    //  0x01000000 -- Enable  TRC

    // Enable counter
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;               // ~0x00000001 -- Disable clock cycle counter
    DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk;               //  0x00000001 -- Enable  clock cycle counter

    // Reset the clock cycle counter value
    DWT->CYCCNT = 0;

    __NOP();    // 3 NO OPERATION instructions
    __NOP();
    __NOP();

    // Verify DWT is actually counting
    if (DWT->CYCCNT == 0) { return false; }

    // Open a fresh measurement window starting from now.
    mWindowStartCycle  = DWT->CYCCNT;
    mWindowSleepCycles = 0;
    mLoopCount         = 0;
    mCpuStats          = {};
    mUpdateAvailable   = false;
    mInitialized       = true;

    return true;
}

/**
 * \brief   Enter sleep mode with native WaitForInterrupt or WaitForEvent
 *          configured. Tracks total elapsed and sleep cycles within the
 *          current ~1-second window; once the window completes, publishes
 *          fresh CpuStats and flags an update available.
 * \param   mode            The sleep mode to configure.
 * \param   suspend_systick Flag, indicating the Systick interrupt is to be
 *                          suspended during sleep or not. Default true. Set
 *                          to false if the caller is willing to trade some
 *                          accounting accuracy for the ~60 cycles spent in
 *                          HAL_SuspendTick + HAL_ResumeTick per call.
 * \note    No-op if Init() has not been called (or returned false).
 * \note    Wake cycles are derived as (window_total - window_sleep) at the
 *          1-second boundary rather than accumulated per-call. This keeps
 *          the hot path to one unsigned subtraction per iteration and
 *          structurally rules out the multiply-overflow bug that the
 *          previous explicit wake-cycle accumulation was prone to under
 *          a 168 MHz PLL clock.
 */
void CpuWakeCounter::EnterSleepMode(SleepMode mode, bool suspend_systick /* = true */)
{
    if (!mInitialized) { return; }

    const uint32_t cycle_before_sleep = DWT->CYCCNT;

    // If requested, suspend Systick to prevent it from waking the CPU
    if (suspend_systick) { HAL_SuspendTick(); }

    // Disable global interrupts, preserve state
    const uint32_t primask_state = __get_PRIMASK();
    __disable_irq();

    // Enter sleep mode: either WaitForInterrupt or WaitForEvent
    if (mode == SleepMode::WaitForInterrupt)
    {
        __WFI();
    }
    else
    {
        __WFE();
    }

    // Snapshot CYCCNT immediately after WFI/WFE returns -- still under
    // PRIMASK so no ISR has run yet, keeping the boundary clean.
    const uint32_t cycle_after_sleep = DWT->CYCCNT;

    // Restore global interrupts to the caller's prior state.
    __set_PRIMASK(primask_state);

    // Note: at this point interrupts are being handled, once finished the
    // remainder of this method continues below.

    // If requested, resume Systick again
    if (suspend_systick) { HAL_ResumeTick(); }

    // Accumulate sleep cycles. Unsigned subtraction handles the (rare)
    // single-wrap case correctly; sub-25-second windows cannot wrap twice
    // at any clock the F407 can run.
    mWindowSleepCycles += (cycle_after_sleep - cycle_before_sleep);

    mUpdateAvailable = false;
    ++mLoopCount;

    // Window total = elapsed cycles since window opened. Once it reaches
    // SystemCoreClock the window is "1 second" and ready to publish.
    const uint32_t total_window = cycle_after_sleep - mWindowStartCycle;
    if (total_window >= SystemCoreClock)
    {
        const uint32_t wake_window = total_window - mWindowSleepCycles;

        mCpuStats.wakePercentage = (100.0f * static_cast<float>(wake_window)) / static_cast<float>(total_window);
        mCpuStats.loopCount      = mLoopCount;

        mUpdateAvailable = true;

        // Start the next window from this same boundary so the timeline is
        // contiguous (no dropped cycles between consecutive windows).
        mWindowStartCycle  = cycle_after_sleep;
        mWindowSleepCycles = 0;
        mLoopCount         = 0;
    }
}

/**
 * \brief   Check if there are updated CpuStats available.
 * \returns True if an update is available, else false.
 */
bool CpuWakeCounter::IsUpdated() const
{
    return mUpdateAvailable;
}

/**
 * \brief   Get the latest published CpuStats.
 * \details Contains the wake percentage and loop count for the most
 *          recently completed measurement window.
 * \returns CpuStats struct (by value).
 */
CpuStats CpuWakeCounter::GetStatistics() const
{
    return mCpuStats;
}
