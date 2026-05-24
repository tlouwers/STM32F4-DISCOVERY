/**
 * \file    TestCpuWakeCounter.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the CpuWakeCounter utility. Drives the real
 *          class against the fake Cortex-M4 DWT cycle counter + intrinsics
 *          (tests/Fake/stm32f4xx_hal.{c,h}).
 *
 * \details The fake models the cycle counter so the firmware logic is testable
 *          on the host: executing instructions (__NOP) and sleeping (__WFI/
 *          __WFE) advance CYCCNT, FakeDWT_SetCounting(0) freezes it (Init's
 *          counter-running probe fails), and FakeDWT_SetSleepAdvance(n) makes
 *          each sleep consume n cycles. SystemCoreClock sets the window length;
 *          manually advancing DWT->CYCCNT between sleeps models awake work.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "gtest/gtest.h"

// Test subject -- the real utility.
#include "utility/CpuWakeCounter/CpuWakeCounter.hpp"


namespace {


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class CpuWakeCounter_Test : public ::testing::Test
{
protected:
    CpuWakeCounter_Test()
    {
        FakeDWT_Reset();
        mSavedCoreClock = SystemCoreClock;
    }

    ~CpuWakeCounter_Test() override
    {
        SystemCoreClock = mSavedCoreClock;      // leave the shared fake as found
        FakeDWT_Reset();
    }

    CpuWakeCounter mSubject;

private:
    uint32_t mSavedCoreClock = 0;
};


/************************************************************************/
/* Init                                                                 */
/************************************************************************/
TEST_F(CpuWakeCounter_Test, Init_DwtCounting_ReturnsTrueAndNoUpdateYet)
{
    EXPECT_TRUE(mSubject.Init());
    EXPECT_FALSE(mSubject.IsUpdated());
}

TEST_F(CpuWakeCounter_Test, Init_DwtNotCounting_ReturnsFalse)
{
    FakeDWT_SetCounting(0);     // CYCCNT stays 0 across the probe NOPs

    EXPECT_FALSE(mSubject.Init());
}

TEST_F(CpuWakeCounter_Test, Init_CalledTwice_ResetsWindowState)
{
    SystemCoreClock = 200;
    ASSERT_TRUE(mSubject.Init());

    FakeDWT_SetSleepAdvance(250);
    mSubject.EnterSleepMode(SleepMode::WaitForInterrupt);   // crosses the window
    ASSERT_TRUE(mSubject.IsUpdated());

    // Re-Init must clear the published-update flag and start a fresh window.
    EXPECT_TRUE(mSubject.Init());
    EXPECT_FALSE(mSubject.IsUpdated());
}


/************************************************************************/
/* EnterSleepMode                                                       */
/************************************************************************/
TEST_F(CpuWakeCounter_Test, EnterSleepMode_NotInitialised_IsNoOp)
{
    mSubject.EnterSleepMode(SleepMode::WaitForInterrupt);

    EXPECT_FALSE(mSubject.IsUpdated());
}

TEST_F(CpuWakeCounter_Test, EnterSleepMode_WindowIncomplete_NoUpdate)
{
    SystemCoreClock = 1000000;          // far larger than one sleep
    ASSERT_TRUE(mSubject.Init());

    FakeDWT_SetSleepAdvance(100);
    mSubject.EnterSleepMode(SleepMode::WaitForInterrupt);

    EXPECT_FALSE(mSubject.IsUpdated());
}

TEST_F(CpuWakeCounter_Test, EnterSleepMode_WindowComplete_PublishesWakePercentage)
{
    SystemCoreClock = 200;
    ASSERT_TRUE(mSubject.Init());       // CYCCNT now 3, window opens here

    FakeDWT_SetSleepAdvance(50);        // each sleep consumes 50 cycles

    // Model "50 awake cycles, then 50 asleep" twice; the second sleep crosses
    // the 200-cycle window boundary -> total 200, sleep 100, wake 100 = 50%.
    DWT->CYCCNT += 50;
    mSubject.EnterSleepMode(SleepMode::WaitForInterrupt);
    EXPECT_FALSE(mSubject.IsUpdated());

    DWT->CYCCNT += 50;
    mSubject.EnterSleepMode(SleepMode::WaitForInterrupt);

    ASSERT_TRUE(mSubject.IsUpdated());
    const CpuStats stats = mSubject.GetStatistics();
    EXPECT_FLOAT_EQ(50.0f, stats.wakePercentage);
    EXPECT_EQ(2U, stats.loopCount);
}

TEST_F(CpuWakeCounter_Test, EnterSleepMode_WaitForEvent_AlsoAccountsSleep)
{
    SystemCoreClock = 100;
    ASSERT_TRUE(mSubject.Init());

    FakeDWT_SetSleepAdvance(150);       // single WFE crosses the window
    mSubject.EnterSleepMode(SleepMode::WaitForEvent);

    EXPECT_TRUE(mSubject.IsUpdated());
    EXPECT_EQ(1U, mSubject.GetStatistics().loopCount);
}

TEST_F(CpuWakeCounter_Test, EnterSleepMode_SystickNotSuspended_StillAccounts)
{
    SystemCoreClock = 100;
    ASSERT_TRUE(mSubject.Init());

    FakeDWT_SetSleepAdvance(150);
    mSubject.EnterSleepMode(SleepMode::WaitForInterrupt, false);   // skip suspend/resume

    EXPECT_TRUE(mSubject.IsUpdated());
}


} // namespace
