/**
 * \file    TestBasicTimer.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the BasicTimer driver. Exercises the real
 *          driver (drivers/drivers/BasicTimer) against the shared fake HAL TIM
 *          surface (tests/Fake/stm32f4xx_hal_tim.{h,cpp}, created by L26 and
 *          extended here with HAL_TIMEx_MasterConfigSynchronization) and the
 *          real shared TimerIRQ dispatcher it registers into.
 *
 * \details BasicTimer is the wired-but-dormant timer (L5b): Start() uses the
 *          CR1.CEN-only HAL_TIM_Base_Start, the update IRQ is never enabled and
 *          the installed TimerIRQ slot never fires, so there is no elapsed
 *          callback to dispatch. Coverage therefore focuses on the driver's own
 *          logic: the Init config-id + frequency validation, the per-instance
 *          SetInstance / clock-gate / IRQn / Slot translation (TIM6/TIM7), the
 *          GetTimerInputClockFreq APB1 prescaler-doubling branch (driven by
 *          writing RCC->CFGR), the CalculatePeriod 16-bit clamp, the
 *          HAL_TIM_Base_Init and master-config (TRGO) failure branches, the
 *          DeInit failure path, and the IsInit/Sleep + Start/IsStarted/Stop
 *          lifecycle incl. the already-started / not-started skip branches. The
 *          register-level HAL behaviour itself is hardware-only and not tested.
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

// Test subject -- the real BasicTimer driver.
#include "drivers/BasicTimer/BasicTimer.hpp"

// Fake HAL control / observation hooks (also pulls in the umbrella for RCC).
extern "C" {
#include "stm32f4xx_hal_tim.h"
}


namespace {


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from BasicTimer::Config::Id(),
// used to prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

BasicTimer::Config ValidConfig()
{
    return BasicTimer::Config(0, 1000);   // 1 kHz, within [20..65535] Hz
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class BasicTimer_Test : public ::testing::Test
{
protected:
    BasicTimer_Test()
        : mSubject(BasicTimerInstance::TIMER_6)
    {
        FakeTIM_Reset();
        RCC->CFGR = 0U;   // default APB1 prescaler /1 (no timer-clock doubling)
    }

    BasicTimer mSubject;
};


/************************************************************************/
/* Init / IsInit                                                        */
/************************************************************************/
TEST_F(BasicTimer_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(BasicTimer_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(BasicTimer_Test, Init_WrongConfigType_ReturnsFalseAndNotInit)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(BasicTimer_Test, Init_ZeroFrequency_ReturnsFalseAndNotInit)
{
    EXPECT_FALSE(mSubject.Init(BasicTimer::Config(0, 0)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(BasicTimer_Test, Init_HalBaseInitFails_ReturnsFalseAndNotInit)
{
    FakeTIM_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(BasicTimer_Test, Init_HalMasterConfigFails_ReturnsFalseAndNotInit)
{
    FakeTIM_SetMasterConfigResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

// TIMER_6 + TIMER_7 cover every SetInstance / clock-gate / IRQn / Slot arm.
TEST_F(BasicTimer_Test, Init_EachInstance_ReturnsTrue)
{
    BasicTimer tim6(BasicTimerInstance::TIMER_6);
    BasicTimer tim7(BasicTimerInstance::TIMER_7);

    EXPECT_TRUE(tim6.Init(ValidConfig()));
    EXPECT_TRUE(tim7.Init(ValidConfig()));
}

// APB1 prescaler /2 -> the timer input clock is doubled (the *2U branch).
TEST_F(BasicTimer_Test, Init_Apb1PrescalerDiv2_ReturnsTrue)
{
    RCC->CFGR = RCC_CFGR_PPRE1_DIV2;

    BasicTimer timer(BasicTimerInstance::TIMER_6);
    EXPECT_TRUE(timer.Init(ValidConfig()));
}

// A very low frequency drives the period above 0xFFFF, exercising the clamp.
TEST_F(BasicTimer_Test, Init_LowFrequency_ClampsPeriodReturnsTrue)
{
    // 1 MHz / 15 - 1 = 66665 > UINT16_MAX -> clamp.
    EXPECT_TRUE(mSubject.Init(BasicTimer::Config(0, 15)));
}


/************************************************************************/
/* Sleep                                                                */
/************************************************************************/
TEST_F(BasicTimer_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(BasicTimer_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}

TEST_F(BasicTimer_Test, Sleep_AfterStart_StopsAndReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Start());

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsStarted());
}


/************************************************************************/
/* Start / IsStarted / Stop                                             */
/************************************************************************/
TEST_F(BasicTimer_Test, Start_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Start());
}

TEST_F(BasicTimer_Test, IsStarted_BeforeStart_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsStarted());
}

TEST_F(BasicTimer_Test, Start_AfterInit_ReturnsTrueAndIsStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Start());
    EXPECT_TRUE(mSubject.IsStarted());
}

TEST_F(BasicTimer_Test, Start_CalledTwice_StaysStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    ASSERT_TRUE(mSubject.Start());
    EXPECT_TRUE(mSubject.Start());   // second call hits the already-started skip
    EXPECT_TRUE(mSubject.IsStarted());
}

TEST_F(BasicTimer_Test, Start_HalStartFails_ReturnsFalseAndNotStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetStartResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Start());
    EXPECT_FALSE(mSubject.IsStarted());
}

TEST_F(BasicTimer_Test, Stop_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Stop());
}

TEST_F(BasicTimer_Test, Stop_HalStopFails_ReturnsFalseAndStaysStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Start());
    FakeTIM_SetStopResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Stop());
    EXPECT_TRUE(mSubject.IsStarted());
}

TEST_F(BasicTimer_Test, Stop_AfterStart_ReturnsTrueAndNotStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Start());

    EXPECT_TRUE(mSubject.Stop());
    EXPECT_FALSE(mSubject.IsStarted());
}

TEST_F(BasicTimer_Test, Stop_WhenNotStarted_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Stop());   // initialised but never started -> skip branch
}


} // namespace
