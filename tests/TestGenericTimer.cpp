/**
 * \file    TestGenericTimer.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the GenericTimer driver. Exercises the real
 *          driver (drivers/drivers/GenericTimer) against the fake HAL TIM
 *          surface (tests/Fake/stm32f4xx_hal_tim.{h,cpp}) and the real shared
 *          TimerIRQ dispatcher (drivers/drivers/TimerIRQ) it registers into.
 *
 * \details Coverage focuses on the driver's own logic: the Init config-id +
 *          frequency validation, the per-instance SetInstance / clock-gate /
 *          IRQn / TimerIRQ-slot translation switches, the GetTimerInputClockFreq
 *          APB1/APB2 + prescaler-doubling branches (driven by writing RCC->CFGR),
 *          the CalculatePeriod 16-bit clamp, the IsInit/Sleep + Start/IsStarted/
 *          Stop lifecycle (incl. the already-started / not-started skip
 *          branches), the HAL_TIM_Base_Init / DeInit failure paths, and the
 *          shared timer vector -> TimerIRQ slot -> HAL_TIM_IRQHandler ->
 *          HAL_TIM_PeriodElapsedCallback elapsed dispatch incl. the post-Sleep
 *          no-dispatch case. The register-level HAL behaviour itself is
 *          hardware-only and not unit-tested.
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

// Test subject -- the real GenericTimer driver.
#include "drivers/GenericTimer/GenericTimer.hpp"

// Fake HAL control / observation hooks (also pulls in the umbrella for RCC).
extern "C" {
#include "stm32f4xx_hal_tim.h"
}


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// TimerIRQ owns the C-linkage timer ISR symbols; the test fires them directly
// to prove a timer vector dispatches into the driver's registered handler.
extern "C" void TIM2_IRQHandler(void);            // TIM2 (APB1) global
extern "C" void TIM1_BRK_TIM9_IRQHandler(void);   // shared TIM1 break + TIM9 global


namespace {


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from GenericTimer::Config::Id(),
// used to prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

GenericTimer::Config ValidConfig()
{
    return GenericTimer::Config(0, 100.0f);
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class GenericTimer_Test : public ::testing::Test
{
protected:
    GenericTimer_Test()
        : mSubject(GenericTimerInstance::TIMER_2)
    {
        FakeTIM_Reset();
        RCC->CFGR    = 0U;   // default APB prescaler /1 (no timer-clock doubling)
    }

    GenericTimer mSubject;
};


/************************************************************************/
/* Init / IsInit                                                        */
/************************************************************************/
TEST_F(GenericTimer_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(GenericTimer_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(GenericTimer_Test, Init_WrongConfigType_ReturnsFalseAndNotInit)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(GenericTimer_Test, Init_ZeroFrequency_ReturnsFalseAndNotInit)
{
    EXPECT_FALSE(mSubject.Init(GenericTimer::Config(0, 0.0f)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(GenericTimer_Test, Init_NegativeFrequency_ReturnsFalseAndNotInit)
{
    EXPECT_FALSE(mSubject.Init(GenericTimer::Config(0, -5.0f)));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(GenericTimer_Test, Init_HalBaseInitFails_ReturnsFalseAndNotInit)
{
    FakeTIM_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

// Exercise every reachable SetInstance / clock-gate / IRQn / Slot switch arm
// (and both arms of GetTimerInputClockFreq -- TIMER_9/10/11 are APB2).
TEST_F(GenericTimer_Test, Init_EveryInstance_ReturnsTrue)
{
    const GenericTimerInstance instances[] = {
        GenericTimerInstance::TIMER_2,  GenericTimerInstance::TIMER_3,
        GenericTimerInstance::TIMER_4,  GenericTimerInstance::TIMER_5,
        GenericTimerInstance::TIMER_9,  GenericTimerInstance::TIMER_10,
        GenericTimerInstance::TIMER_11, GenericTimerInstance::TIMER_12,
        GenericTimerInstance::TIMER_13, GenericTimerInstance::TIMER_14
    };
    for (const auto& inst : instances)
    {
        GenericTimer timer(inst);
        EXPECT_TRUE(timer.Init(ValidConfig()));
    }
}

// APB1 prescaler /2 -> timer input clock is doubled (the *2U branch).
TEST_F(GenericTimer_Test, Init_Apb1PrescalerDiv2_ReturnsTrue)
{
    RCC->CFGR = RCC_CFGR_PPRE1_DIV2;

    GenericTimer timer(GenericTimerInstance::TIMER_2);
    EXPECT_TRUE(timer.Init(ValidConfig()));
}

// APB2 prescaler /2 -> the APB2 timer-clock doubling branch (TIMER_9..11).
TEST_F(GenericTimer_Test, Init_Apb2PrescalerDiv2_ReturnsTrue)
{
    RCC->CFGR = RCC_CFGR_PPRE2_DIV2;

    GenericTimer timer(GenericTimerInstance::TIMER_9);
    EXPECT_TRUE(timer.Init(ValidConfig()));
}

// A very low frequency on a 16-bit timer drives the period above 0xFFFF,
// exercising the CalculatePeriod clamp.
TEST_F(GenericTimer_Test, Init_LowFrequencyOn16BitTimer_ClampsPeriodReturnsTrue)
{
    GenericTimer timer(GenericTimerInstance::TIMER_3);   // 16-bit
    EXPECT_TRUE(timer.Init(GenericTimer::Config(0, 0.1f)));
}

// A frequency above the 10 kHz CK_CNT ceiling is rejected (cannot be represented).
TEST_F(GenericTimer_Test, Init_FrequencyAboveCeiling_ReturnsFalseAndNotInit)
{
    EXPECT_FALSE(mSubject.Init(GenericTimer::Config(0, 20000.0f)));
    EXPECT_FALSE(mSubject.IsInit());
}


/************************************************************************/
/* Sleep                                                                */
/************************************************************************/
TEST_F(GenericTimer_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(GenericTimer_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}

TEST_F(GenericTimer_Test, Sleep_AfterStart_StopsAndReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Start([]() {}));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsStarted());
}


/************************************************************************/
/* Start / IsStarted / Stop                                             */
/************************************************************************/
TEST_F(GenericTimer_Test, Start_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Start([]() {}));
}

TEST_F(GenericTimer_Test, IsStarted_BeforeStart_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsStarted());
}

TEST_F(GenericTimer_Test, Start_AfterInit_ReturnsTrueAndIsStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Start([]() {}));
    EXPECT_TRUE(mSubject.IsStarted());
}

TEST_F(GenericTimer_Test, Start_CalledTwice_StaysStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    ASSERT_TRUE(mSubject.Start([]() {}));
    EXPECT_TRUE(mSubject.Start([]() {}));   // second call hits the already-started skip
    EXPECT_TRUE(mSubject.IsStarted());
}

TEST_F(GenericTimer_Test, Start_HalStartFails_ReturnsFalseAndNotStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetStartResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Start([]() {}));
    EXPECT_FALSE(mSubject.IsStarted());
}

TEST_F(GenericTimer_Test, Stop_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Stop());
}

TEST_F(GenericTimer_Test, Stop_HalStopFails_ReturnsFalseAndStaysStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Start([]() {}));
    FakeTIM_SetStopResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Stop());
    EXPECT_TRUE(mSubject.IsStarted());
}

TEST_F(GenericTimer_Test, Stop_AfterStart_ReturnsTrueAndNotStarted)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.Start([]() {}));

    EXPECT_TRUE(mSubject.Stop());
    EXPECT_FALSE(mSubject.IsStarted());
}

TEST_F(GenericTimer_Test, Stop_WhenNotStarted_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Stop());   // initialised but never started -> skip branch
}


/************************************************************************/
/* IRQ dispatch / period-elapsed                                        */
/************************************************************************/
TEST_F(GenericTimer_Test, IRQHandler_AfterStart_DispatchesElapsed)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    bool wasCalled = false;
    ASSERT_TRUE(mSubject.Start([&]() { wasCalled = true; }));
    ASSERT_EQ(0, FakeTIM_IRQHandlerCallCount());

    // Firing the TIM2 vector must route through the registered TimerIRQ slot
    // into HAL_TIM_IRQHandler, which models the update event and dispatches the
    // period-elapsed callback.
    TIM2_IRQHandler();

    EXPECT_EQ(1, FakeTIM_IRQHandlerCallCount());
    EXPECT_TRUE(wasCalled);
}

TEST_F(GenericTimer_Test, IRQHandler_Timer9SharedVector_DispatchesElapsed)
{
    GenericTimer timer9(GenericTimerInstance::TIMER_9);
    ASSERT_TRUE(timer9.Init(ValidConfig()));

    bool wasCalled = false;
    ASSERT_TRUE(timer9.Start([&]() { wasCalled = true; }));

    // TIM9 shares the TIM1-break vector; the dispatcher must still reach it.
    TIM1_BRK_TIM9_IRQHandler();

    EXPECT_TRUE(wasCalled);
}

TEST_F(GenericTimer_Test, IRQHandler_AfterSleep_DoesNotDispatch)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    bool wasCalled = false;
    ASSERT_TRUE(mSubject.Start([&]() { wasCalled = true; }));
    ASSERT_TRUE(mSubject.Sleep());   // Uninstall() drops the TimerIRQ slot.

    TIM2_IRQHandler();

    EXPECT_EQ(0, FakeTIM_IRQHandlerCallCount());
    EXPECT_FALSE(wasCalled);
}


} // namespace
