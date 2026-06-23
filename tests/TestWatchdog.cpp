/**
 * \file    TestWatchdog.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the Watchdog (IWDG) driver. Exercises the real
 *          driver (drivers/drivers/Watchdog) against the fake HAL IWDG surface
 *          (tests/Fake/stm32f4xx_hal_iwdg.{h,cpp}).
 *
 * \details Coverage focuses on the driver's own logic: the Init LSI-ready guard,
 *          the config-id guard, the HAL_IWDG_Init failure branch, and the full
 *          per-timeout prescaler/reload translation (every Timeout enum value
 *          exercises both private switch tables); the IsInit lifecycle; the
 *          Sleep contract (the IWDG cannot be stopped, so it always returns
 *          false); and Refresh() reaching HAL_IWDG_Refresh. The IWDG register
 *          behaviour itself (and the unconditional HAL_OK from HAL_IWDG_Refresh,
 *          per L5a) is hardware-only and not unit-tested.
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

// Test subject -- the real Watchdog driver.
#include "drivers/Watchdog/Watchdog.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_iwdg.h"
}


namespace {

/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from Watchdog::Config::Id(), used
// to prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class Watchdog_Test : public ::testing::Test
{
protected:
    Watchdog_Test()
    {
        FakeIWDG_Reset();   // LSI ready, Init OK, refresh count 0 per test.
    }

    Watchdog mSubject;
};


/************************************************************************/
/* Init / IsInit                                                        */
/************************************************************************/
TEST_F(Watchdog_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Watchdog_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(Watchdog::Config()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(Watchdog_Test, Init_WrongConfigType_ReturnsFalseAndNotInit)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

// LSI not ready short-circuits before the config-id check; the ASSERT is a
// no-op in the native build (see Fake/utility/Assert), so Init returns false.
TEST_F(Watchdog_Test, Init_LsiNotReady_ReturnsFalseAndNotInit)
{
    FakeIWDG_SetLSIReady(0);

    EXPECT_FALSE(mSubject.Init(Watchdog::Config()));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Watchdog_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeIWDG_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(Watchdog::Config()));
    EXPECT_FALSE(mSubject.IsInit());
}

// Each Timeout drives CalculatePrescaler + CalculateReload; iterate every enum
// value to exercise all reachable switch arms in both translators.
TEST_F(Watchdog_Test, Init_EveryTimeout_ReturnsTrue)
{
    const Watchdog::Timeout timeouts[] = {
        Watchdog::Timeout::_5_MS,   Watchdog::Timeout::_10_MS,
        Watchdog::Timeout::_25_MS,  Watchdog::Timeout::_50_MS,
        Watchdog::Timeout::_125_MS, Watchdog::Timeout::_250_MS,
        Watchdog::Timeout::_500_MS, Watchdog::Timeout::_1_S,
        Watchdog::Timeout::_2_S,    Watchdog::Timeout::_4_S,
        Watchdog::Timeout::_8_S,    Watchdog::Timeout::_16_S,
        Watchdog::Timeout::_32_S
    };
    for (const auto& t : timeouts)
    {
        FakeIWDG_Reset();
        Watchdog wdg;
        EXPECT_TRUE(wdg.Init(Watchdog::Config(t)));
    }
}


/************************************************************************/
/* Sleep                                                                */
/************************************************************************/
TEST_F(Watchdog_Test, Sleep_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.Sleep());
}

// The IWDG cannot be disabled once started, so Sleep returns false even after a
// successful Init.
TEST_F(Watchdog_Test, Sleep_AfterInit_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(Watchdog::Config()));
    EXPECT_FALSE(mSubject.Sleep());
}


/************************************************************************/
/* Refresh                                                              */
/************************************************************************/
TEST_F(Watchdog_Test, Refresh_AfterInit_CallsHalRefresh)
{
    ASSERT_TRUE(mSubject.Init(Watchdog::Config()));
    ASSERT_EQ(0, FakeIWDG_RefreshCallCount());

    mSubject.Refresh();

    EXPECT_EQ(1, FakeIWDG_RefreshCallCount());
}

// Refresh before Init must not touch the HAL: the handle is still null.
TEST_F(Watchdog_Test, Refresh_BeforeInit_DoesNotCallHalRefresh)
{
    mSubject.Refresh();

    EXPECT_EQ(0, FakeIWDG_RefreshCallCount());
}


} // namespace
