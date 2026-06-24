/**
 * \file    TestRtc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the Rtc driver. Exercises the real driver
 *          (drivers/drivers/Rtc) against the fake HAL RTC surface
 *          (tests/Fake/stm32f4xx_hal_rtc.{h,cpp} + stm32f4xx_hal_rtc_ex.h +
 *          stm32f4xx_hal_pwr.h).
 *
 * \details Coverage focuses on the driver's own logic: the Init config-id
 *          guard, each ClockSource switch arm, the HAL_RTC_Init failure branch,
 *          and the cold/warm-boot magic detection (cold boot seeds a placeholder
 *          and stamps BKP_DR0; warm boot leaves a running clock untouched; a
 *          failed seed leaves the magic unstamped); the IsInit/Sleep lifecycle
 *          (Sleep is non-destructive -- it never DeInits the running clock) and
 *          the explicit Deinitialize() teardown incl. its HAL_RTC_DeInit failure
 *          branch; SetDateTime's full per-field
 *          range-guard block plus both HAL set-failure points; GetDateTime's
 *          not-init guard, the set/get round-trip, and both HAL get-failure
 *          points. The BCD register behaviour itself is hardware-only and not
 *          unit-tested.
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

// Test subject -- the real Rtc driver.
#include "drivers/Rtc/Rtc.hpp"

// Fake HAL control / observation hooks.
extern "C" {
#include "stm32f4xx_hal_rtc.h"
#include "stm32f4xx_hal_rtc_ex.h"
}


namespace {

// Mirrors the (private) RTC_BKP_MAGIC sentinel in Rtc.cpp: present in BKP_DR0
// means a battery-backed clock is already running (warm boot); absent means a
// cold boot. Kept in sync with the driver constant by hand.
constexpr uint32_t RTC_BKP_MAGIC = 0x52544301;

// The cold-boot placeholder year Rtc::Init seeds when no magic is found.
constexpr uint16_t COLD_BOOT_SEED_YEAR = 2021;


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
// A foreign IConfig whose ConfigId() differs from Rtc::Config::Id(), used to
// prove Init() rejects a config of the wrong concrete type.
struct ForeignConfig : public IConfig
{
    static const void* Id() { static const char sTag = 0; return &sTag; }
    const void* ConfigId() const override { return Id(); }
};

Rtc::Config ValidConfig()
{
    return Rtc::Config(Rtc::ClockSource::LSE);
}

DateTime MakeDateTime(uint16_t year, uint8_t month, uint8_t day,
                      uint8_t hour, uint8_t minute, uint8_t second)
{
    DateTime dt;
    dt.year   = year;
    dt.month  = month;
    dt.day    = day;
    dt.hour   = hour;
    dt.minute = minute;
    dt.second = second;
    return dt;
}


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class Rtc_Test : public ::testing::Test
{
protected:
    Rtc_Test()
    {
        FakeRTC_Reset();   // Fresh, cold (no magic) backup domain per test.
    }

    Rtc mSubject;
};


/************************************************************************/
/* Init / IsInit                                                        */
/************************************************************************/
TEST_F(Rtc_Test, IsInit_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Rtc_Test, Init_ValidConfig_ReturnsTrueAndIsInit)
{
    EXPECT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.IsInit());
}

TEST_F(Rtc_Test, Init_WrongConfigType_ReturnsFalseAndNotInit)
{
    ForeignConfig foreign;
    EXPECT_FALSE(mSubject.Init(foreign));
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Rtc_Test, Init_HalInitFails_ReturnsFalseAndNotInit)
{
    FakeRTC_SetInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.IsInit());
}

// Exercise every reachable arm of EnablePeripheralClock's clock-source switch.
TEST_F(Rtc_Test, Init_EachClockSource_ReturnsTrue)
{
    const Rtc::ClockSource sources[] = {
        Rtc::ClockSource::LSI, Rtc::ClockSource::LSE, Rtc::ClockSource::HSE
    };
    for (const auto& src : sources)
    {
        FakeRTC_Reset();
        Rtc rtc;
        EXPECT_TRUE(rtc.Init(Rtc::Config(src)));
    }
}


/************************************************************************/
/* Cold / warm boot detection                                          */
/************************************************************************/
TEST_F(Rtc_Test, Init_ColdBoot_SeedsPlaceholderAndStampsMagic)
{
    // FakeRTC_Reset() left BKP_DR0 empty -> cold boot.
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.WasColdBoot());
    EXPECT_EQ(RTC_BKP_MAGIC, FakeRTC_GetBackupRegister(RTC_BKP_DR0));

    // The seeded placeholder time is now readable.
    DateTime dt;
    ASSERT_TRUE(mSubject.GetDateTime(dt));
    EXPECT_EQ(COLD_BOOT_SEED_YEAR, dt.year);
    EXPECT_EQ(1, dt.month);
    EXPECT_EQ(1, dt.day);
}

TEST_F(Rtc_Test, Init_WarmBoot_DoesNotReseedAndReportsNotColdBoot)
{
    // Magic already present -> a battery-backed clock is running.
    FakeRTC_SetBackupRegister(RTC_BKP_DR0, RTC_BKP_MAGIC);

    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_FALSE(mSubject.WasColdBoot());

    // Init must not have seeded the 2021 placeholder over the running clock;
    // the fake's stored year is still the reset default (-> 2000).
    DateTime dt;
    ASSERT_TRUE(mSubject.GetDateTime(dt));
    EXPECT_NE(COLD_BOOT_SEED_YEAR, dt.year);
}

// Cold boot where seeding the placeholder fails: Init still succeeds, but the
// magic must NOT be stamped (so the seed is retried on the next boot).
TEST_F(Rtc_Test, Init_ColdBootSeedFails_DoesNotStampMagic)
{
    FakeRTC_SetSetTimeResult(HAL_ERROR);

    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.WasColdBoot());
    EXPECT_NE(RTC_BKP_MAGIC, FakeRTC_GetBackupRegister(RTC_BKP_DR0));
}

TEST_F(Rtc_Test, WasColdBoot_BeforeInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.WasColdBoot());
}


/************************************************************************/
/* Sleep                                                                */
/************************************************************************/
TEST_F(Rtc_Test, Sleep_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

// Sleep() must NOT de-initialise a running RTC -- the backup-domain clock keeps
// ticking so battery-backed time survives. Proof: even with HAL_RTC_DeInit rigged
// to fail, Sleep() still succeeds because it never calls it.
TEST_F(Rtc_Test, Sleep_DoesNotDeInit_ReturnsTrueEvenIfHalDeInitWouldFail)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeRTC_SetDeInitResult(HAL_ERROR);

    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}


/************************************************************************/
/* Deinitialize (explicit destructive teardown)                         */
/************************************************************************/
TEST_F(Rtc_Test, Deinitialize_AfterInit_ReturnsTrueAndNotInit)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));

    EXPECT_TRUE(mSubject.Deinitialize());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Rtc_Test, Deinitialize_BeforeInit_ReturnsTrue)
{
    EXPECT_TRUE(mSubject.Deinitialize());
}

TEST_F(Rtc_Test, Deinitialize_HalDeInitFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeRTC_SetDeInitResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.Deinitialize());
}


/************************************************************************/
/* SetDateTime                                                          */
/************************************************************************/
TEST_F(Rtc_Test, SetDateTime_NotInit_ReturnsFalse)
{
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_ValidAfterInit_ReturnsTrue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_TRUE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_YearBelowRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(1999, 6, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_YearAboveRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2100, 6, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_MonthZero_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 0, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_MonthAboveRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 13, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_DayZero_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 0, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_DayAboveRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 32, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_HourAboveRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 24, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_MinuteAboveRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 60, 45)));
}

TEST_F(Rtc_Test, SetDateTime_SecondAboveRange_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 30, 60)));
}

TEST_F(Rtc_Test, SetDateTime_HalSetTimeFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeRTC_SetSetTimeResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 30, 45)));
}

TEST_F(Rtc_Test, SetDateTime_HalSetDateFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeRTC_SetSetDateResult(HAL_ERROR);

    EXPECT_FALSE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 30, 45)));
}


/************************************************************************/
/* GetDateTime                                                          */
/************************************************************************/
TEST_F(Rtc_Test, GetDateTime_NotInit_ReturnsFalse)
{
    DateTime dt;
    EXPECT_FALSE(mSubject.GetDateTime(dt));
}

TEST_F(Rtc_Test, GetDateTime_AfterSet_RoundTripsTheValue)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    ASSERT_TRUE(mSubject.SetDateTime(MakeDateTime(2024, 6, 15, 12, 30, 45)));

    DateTime dt;
    ASSERT_TRUE(mSubject.GetDateTime(dt));
    EXPECT_EQ(2024, dt.year);
    EXPECT_EQ(6,    dt.month);
    EXPECT_EQ(15,   dt.day);
    EXPECT_EQ(12,   dt.hour);
    EXPECT_EQ(30,   dt.minute);
    EXPECT_EQ(45,   dt.second);
}

TEST_F(Rtc_Test, GetDateTime_HalGetTimeFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeRTC_SetGetTimeResult(HAL_ERROR);

    DateTime dt;
    EXPECT_FALSE(mSubject.GetDateTime(dt));
}

TEST_F(Rtc_Test, GetDateTime_HalGetDateFails_ReturnsFalse)
{
    ASSERT_TRUE(mSubject.Init(ValidConfig()));
    FakeRTC_SetGetDateResult(HAL_ERROR);

    DateTime dt;
    EXPECT_FALSE(mSubject.GetDateTime(dt));
}


} // namespace
