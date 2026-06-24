/**
 * \file    TestPWM.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the PWM driver. Exercises the real driver
 *          (drivers/drivers/PWM) against the shared fake HAL TIM surface
 *          (tests/Fake/stm32f4xx_hal_tim.{h,cpp}, created by L26 and extended
 *          here with the HAL_TIM_PWM_* output-compare path).
 *
 * \details PWM has no interrupt/callback path -- it is a pure output driver on
 *          TIM2..5. Coverage focuses on the driver's own logic: the Init
 *          config-id validation + HAL_TIM_PWM_Init failure branch, the
 *          per-instance SetInstance / clock-gate switches, the
 *          GetTimerInputClockFreq APB1 prescaler-doubling branch (driven by
 *          writing RCC->CFGR), the CalculatePeriod clamp (period 0 / above
 *          0xFFFF), the CalculatePulse duty-cycle clamp + 0%-special-case, the
 *          GetChannel translation (all four channels), ConfigureChannel /
 *          SetDutyCycle / Start / Stop incl. their not-init guards and
 *          HAL-failure branches, and the Sleep (StopAllChannels + DeInit)
 *          path. The register-level HAL behaviour itself is hardware-only and
 *          not unit-tested.
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

// Test subject -- the real PWM driver.
#include "drivers/PWM/PWM.hpp"

// Fake HAL control / observation hooks (also pulls in the umbrella for RCC).
extern "C" {
#include "stm32f4xx_hal_tim.h"
}


namespace {

using Ch = IPWM::Channel;


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from PWM::Config::Id(), used to
// prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

PWM::Config ValidConfig()
{
    return PWM::Config(1000.0f);   // 1 kHz
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class PWM_Test : public ::testing::Test
{
protected:
    PWM_Test()
        : mSubject(PwmTimerInstance::TIMER_2)
    {
        FakeTIM_Reset();
        RCC->CFGR = 0U;   // default APB1 prescaler /1 (no timer-clock doubling)
    }

    PWM mSubject;
};


/************************************************************************/
/* Init / IsInit                                                        */
/************************************************************************/
TEST_F(PWM_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(PWM_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(PWM_Test, Init_WrongConfigType_ReturnsFalseAndNotInit)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(PWM_Test, Init_HalPwmInitFails_ReturnsFalseAndNotInit)
{
    FakeTIM_SetPwmInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

// TIMER_2..5 cover every SetInstance / clock-gate switch arm.
TEST_F(PWM_Test, Init_EachInstance_ReturnsTrue)
{
    PWM tim2(PwmTimerInstance::TIMER_2);
    PWM tim3(PwmTimerInstance::TIMER_3);
    PWM tim4(PwmTimerInstance::TIMER_4);
    PWM tim5(PwmTimerInstance::TIMER_5);

    EXPECT_TRUE(tim2.Init(ValidConfig()));
    EXPECT_TRUE(tim3.Init(ValidConfig()));
    EXPECT_TRUE(tim4.Init(ValidConfig()));
    EXPECT_TRUE(tim5.Init(ValidConfig()));
}

// APB1 prescaler /2 -> the timer input clock is doubled (the *2U branch).
TEST_F(PWM_Test, Init_Apb1PrescalerDiv2_ReturnsTrue)
{
    RCC->CFGR = RCC_CFGR_PPRE1_DIV2;

    PWM timer(PwmTimerInstance::TIMER_2);
    EXPECT_TRUE(timer.Init(ValidConfig()));
}

// A very low frequency drives the period above 0xFFFF -> CalculatePeriod clamp.
TEST_F(PWM_Test, Init_LowFrequency_ClampsPeriodReturnsTrue)
{
    EXPECT_TRUE(mSubject.Init(PWM::Config(100.0f)));   // 42 MHz / 100 - 1 > UINT16_MAX
}

// A frequency equal to the timer input clock makes the period compute to 0 ->
// the other arm of the CalculatePeriod clamp.
TEST_F(PWM_Test, Init_FrequencyEqualsTimerClock_ClampsPeriodReturnsTrue)
{
    EXPECT_TRUE(mSubject.Init(PWM::Config(42000000.0f)));   // 42 MHz / 42 MHz - 1 == 0
}


/************************************************************************/
/* Sleep                                                                */
/************************************************************************/
TEST_F(PWM_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(PWM_Test, Sleep_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetPwmDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Sleep());
}


/************************************************************************/
/* ConfigureChannel                                                     */
/************************************************************************/
TEST_F(PWM_Test, ConfigureChannel_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_1, 0.5f)));
}

TEST_F(PWM_Test, ConfigureChannel_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_1, 0.5f)));
}

TEST_F(PWM_Test, ConfigureChannel_LowPolarity_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_1, 0.5f, PWM::Polarity::LOW)));
}

TEST_F(PWM_Test, ConfigureChannel_HalConfigFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetPwmConfigChannelResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_1, 0.5f)));
}

// Configure all four channels -> exercise every GetChannel switch arm.
TEST_F(PWM_Test, ConfigureChannel_EveryChannel_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_1, 0.25f)));
    EXPECT_TRUE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_2, 0.50f)));
    EXPECT_TRUE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_3, 0.75f)));
    EXPECT_TRUE(mSubject.ConfigureChannel(PWM::ChannelConfig(Ch::CHANNEL_4, 1.00f)));
}


/************************************************************************/
/* SetDutyCycle (CalculatePulse branches)                              */
/************************************************************************/
TEST_F(PWM_Test, SetDutyCycle_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.SetDutyCycle(Ch::CHANNEL_1, 0.5f));
}

TEST_F(PWM_Test, SetDutyCycle_HalfDuty_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.SetDutyCycle(Ch::CHANNEL_1, 0.5f));
}

TEST_F(PWM_Test, SetDutyCycle_ZeroDuty_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.SetDutyCycle(Ch::CHANNEL_1, 0.0f));   // CalculatePulse 0%-special-case
}

TEST_F(PWM_Test, SetDutyCycle_FullDuty_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.SetDutyCycle(Ch::CHANNEL_1, 1.0f));
}

TEST_F(PWM_Test, SetDutyCycle_DutyAboveOne_ClampsReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.SetDutyCycle(Ch::CHANNEL_1, 1.5f));   // clamp > 1.0
}

TEST_F(PWM_Test, SetDutyCycle_NegativeDuty_ClampsReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.SetDutyCycle(Ch::CHANNEL_1, -0.5f));  // clamp < 0.0
}


/************************************************************************/
/* Start / Stop                                                         */
/************************************************************************/
TEST_F(PWM_Test, Start_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Start(Ch::CHANNEL_1));
}

TEST_F(PWM_Test, Start_AfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.Start(Ch::CHANNEL_1));
}

TEST_F(PWM_Test, Start_HalStartFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetPwmStartResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Start(Ch::CHANNEL_1));
}

TEST_F(PWM_Test, Stop_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Stop(Ch::CHANNEL_1));
}

TEST_F(PWM_Test, Stop_AfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.Stop(Ch::CHANNEL_1));
}

TEST_F(PWM_Test, Stop_HalStopFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeTIM_SetPwmStopResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Stop(Ch::CHANNEL_1));
}


} // namespace
