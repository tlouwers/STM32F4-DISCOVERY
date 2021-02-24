/**
 * \file CpuWakeCounter.cpp
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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/utility/CpuWakeCounter
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    02-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "CpuWakeCounter.hpp"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Enter sleep mode with native WaitForInterrupt or WaitForEvent
 *          configured. Will keep track of the CPU wake percentage and
 *          (main) loop count. Once per second an update is flagged available
 *          in variable 'updated'.
 * \param   mode            The sleep mode to configure.
 * \param   suspend_systick Flag, indicating the Systick interrupt is to be
 *                          suspended during sleep or not. Default true.
 * \note    This method assumes the DWT block is enabled.
 * \note    This method assumes it is called roughly once per second (or more).
 */
void CpuWakeCounter::EnterSleepMode(SleepMode mode, bool suspend_systick /* = true */)
{
    static uint32_t CycleCountAfterSleep = 0;
    static uint32_t WakeCycleCount       = 0;
    static uint32_t SleepCycleCount      = 0;
    static uint32_t LoopCount            = 0;

    // Save the CPU counts from before entering sleep mode
    uint32_t CycleCountBeforeSleep = DWT->CYCCNT;

    // Increase the wake cycle count - take wrapping into account
    if (CycleCountBeforeSleep >= CycleCountAfterSleep)
    {
        WakeCycleCount += (CycleCountBeforeSleep - CycleCountAfterSleep);
    }
    else
    {
        WakeCycleCount += (UINT32_MAX - (CycleCountAfterSleep - CycleCountBeforeSleep - 1));
    }

    // If requested, suspend Systick to prevent it from waking the CPU
    if (suspend_systick) { HAL_SuspendTick(); }

    // Disable global interrupts, preserve state
    uint32_t primask_state = __get_PRIMASK();
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

    // This is right after waking from event/interrupt

    // Save the CPU counts from right after exiting sleep mode
    CycleCountAfterSleep = DWT->CYCCNT;

    // Restore global interrupts - restore state
    if (!primask_state) { __enable_irq(); }

    // Note: at this point interrupts are being handled, once finished the
    // remainder of this method continues below.

    // If requested, resume Systick again
    if (suspend_systick) { HAL_ResumeTick(); }

    // Increase the sleep cycle count - take wrapping into account
    if (CycleCountAfterSleep >= CycleCountBeforeSleep)
    {
        SleepCycleCount += (CycleCountAfterSleep - CycleCountBeforeSleep);
    }
    else
    {
        SleepCycleCount += (UINT32_MAX - (CycleCountBeforeSleep - CycleCountAfterSleep - 1));
    }

    // Flag no update is available (yet)
    mUpdateAvailable = false;

    // Increase the loop counter
    ++LoopCount;

    // If the time spent awake and asleep together is more than 1 second in CPU cycles
    if ( (WakeCycleCount + SleepCycleCount) >= SystemCoreClock)
    {
        mCpuStats.loopCount       = LoopCount;
        // Calculate percentage - assume we iterate once per second
        mCpuStats.wakePercentage  = WakeCycleCount * 100;
        mCpuStats.wakePercentage /= WakeCycleCount + SleepCycleCount;

        mUpdateAvailable = true;

        SleepCycleCount = 0;     // Reset counters
        WakeCycleCount  = 0;
        LoopCount       = 0;
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
 * \brief   Get the updated CpuStats.
 * \details Contains the wake percentage and loop count.
 * \returns Update CpuStats as struct.
 */
CpuStats CpuWakeCounter::GetStatistics() const
{
    CpuStats stats;

    stats.loopCount      = mCpuStats.loopCount;
    stats.wakePercentage = mCpuStats.wakePercentage;

    return stats;
}
