/**
 * \file    TestTimerIRQ.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the TimerIRQ dispatcher (drivers/drivers/
 *          TimerIRQ). Pure software vector owner -- no HAL, no fakes.
 *
 * \details TimerIRQ owns the only copy of every STM32F407 timer ISR symbol and
 *          fans each (often shared) vector out to a per-timer std::function
 *          slot. The tests register a counting handler on a slot, fire the
 *          corresponding extern-C ISR entry point directly, and assert the
 *          correct slot(s) ran. Coverage: Install / Uninstall (incl. the
 *          out-of-range slot guard), every ISR symbol -> its slot(s) (incl. the
 *          four TIM1 and four TIM8 shared vectors and the TIM6/DAC-underrun
 *          shared vector), double-register overwrite, and the empty-slot /
 *          post-Uninstall safe no-op path.
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

// Test subject -- the real TimerIRQ dispatcher.
#include "drivers/TimerIRQ/TimerIRQ.hpp"


/************************************************************************/
/* External references                                                  */
/************************************************************************/
// The C-linkage timer ISR symbols TimerIRQ defines; fired directly by the tests.
extern "C" {
void TIM1_BRK_TIM9_IRQHandler(void);
void TIM1_UP_TIM10_IRQHandler(void);
void TIM1_TRG_COM_TIM11_IRQHandler(void);
void TIM1_CC_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void TIM5_IRQHandler(void);
void TIM6_DAC_IRQHandler(void);
void TIM7_IRQHandler(void);
void TIM8_BRK_TIM12_IRQHandler(void);
void TIM8_UP_TIM13_IRQHandler(void);
void TIM8_TRG_COM_TIM14_IRQHandler(void);
void TIM8_CC_IRQHandler(void);
}


namespace {

using TimerIRQ::Slot;


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
// The dispatch slot table is process-global static state, so every test
// starts and ends with all slots cleared to avoid cross-test contamination.
class TimerIRQ_Test : public ::testing::Test
{
protected:
    void SetUp() override    { ResetAllSlots(); }
    void TearDown() override { ResetAllSlots(); }

    static void ResetAllSlots()
    {
        for (uint8_t s = 0; s <= static_cast<uint8_t>(Slot::DAC_UNDERRUN); ++s)
        {
            TimerIRQ::Uninstall(static_cast<Slot>(s));
        }
    }
};


/************************************************************************/
/* Install / Uninstall                                                  */
/************************************************************************/
TEST_F(TimerIRQ_Test, Install_ValidSlot_ReturnsTrue)
{
    EXPECT_TRUE(TimerIRQ::Install(Slot::TIMER_2, []() {}));
}

TEST_F(TimerIRQ_Test, Install_OutOfRangeSlot_ReturnsFalse)
{
    const Slot bad = static_cast<Slot>(static_cast<uint8_t>(Slot::DAC_UNDERRUN) + 1);
    EXPECT_FALSE(TimerIRQ::Install(bad, []() {}));
}

TEST_F(TimerIRQ_Test, Uninstall_ValidSlot_ReturnsTrue)
{
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_2, []() {}));
    EXPECT_TRUE(TimerIRQ::Uninstall(Slot::TIMER_2));
}

TEST_F(TimerIRQ_Test, Uninstall_OutOfRangeSlot_ReturnsFalse)
{
    const Slot bad = static_cast<Slot>(static_cast<uint8_t>(Slot::DAC_UNDERRUN) + 1);
    EXPECT_FALSE(TimerIRQ::Uninstall(bad));
}


/************************************************************************/
/* Dispatch behaviour                                                   */
/************************************************************************/
TEST_F(TimerIRQ_Test, Dispatch_RegisteredSlot_InvokesHandler)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_2, [&]() { called = true; }));

    TIM2_IRQHandler();

    EXPECT_TRUE(called);
}

TEST_F(TimerIRQ_Test, Dispatch_UnregisteredSlot_IsNoOp)
{
    // Nothing installed on TIMER_3 -> firing its vector must be a safe no-op.
    TIM3_IRQHandler();
    SUCCEED();
}

TEST_F(TimerIRQ_Test, Dispatch_AfterUninstall_DoesNotInvoke)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_2, [&]() { called = true; }));
    ASSERT_TRUE(TimerIRQ::Uninstall(Slot::TIMER_2));

    TIM2_IRQHandler();

    EXPECT_FALSE(called);
}

TEST_F(TimerIRQ_Test, Install_TwiceSameSlot_LatestHandlerWins)
{
    bool firstCalled  = false;
    bool secondCalled = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_2, [&]() { firstCalled = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_2, [&]() { secondCalled = true; }));

    TIM2_IRQHandler();

    EXPECT_FALSE(firstCalled);
    EXPECT_TRUE(secondCalled);
}


/************************************************************************/
/* Per-vector routing -- dedicated (non-shared) vectors                */
/************************************************************************/
TEST_F(TimerIRQ_Test, Tim2Vector_FiresTimer2Slot)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_2, [&]() { called = true; }));
    TIM2_IRQHandler();
    EXPECT_TRUE(called);
}

TEST_F(TimerIRQ_Test, Tim3Vector_FiresTimer3Slot)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_3, [&]() { called = true; }));
    TIM3_IRQHandler();
    EXPECT_TRUE(called);
}

TEST_F(TimerIRQ_Test, Tim4Vector_FiresTimer4Slot)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_4, [&]() { called = true; }));
    TIM4_IRQHandler();
    EXPECT_TRUE(called);
}

TEST_F(TimerIRQ_Test, Tim5Vector_FiresTimer5Slot)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_5, [&]() { called = true; }));
    TIM5_IRQHandler();
    EXPECT_TRUE(called);
}

TEST_F(TimerIRQ_Test, Tim7Vector_FiresTimer7Slot)
{
    bool called = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_7, [&]() { called = true; }));
    TIM7_IRQHandler();
    EXPECT_TRUE(called);
}


/************************************************************************/
/* Per-vector routing -- shared TIM1 vectors                           */
/************************************************************************/
TEST_F(TimerIRQ_Test, Tim1Brk_Tim9Vector_FiresTimer1AndTimer9)
{
    bool t1 = false, t9 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_1, [&]() { t1 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_9, [&]() { t9 = true; }));

    TIM1_BRK_TIM9_IRQHandler();

    EXPECT_TRUE(t1);
    EXPECT_TRUE(t9);
}

TEST_F(TimerIRQ_Test, Tim1Up_Tim10Vector_FiresTimer1AndTimer10)
{
    bool t1 = false, t10 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_1, [&]() { t1 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_10, [&]() { t10 = true; }));

    TIM1_UP_TIM10_IRQHandler();

    EXPECT_TRUE(t1);
    EXPECT_TRUE(t10);
}

TEST_F(TimerIRQ_Test, Tim1TrgCom_Tim11Vector_FiresTimer1AndTimer11)
{
    bool t1 = false, t11 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_1, [&]() { t1 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_11, [&]() { t11 = true; }));

    TIM1_TRG_COM_TIM11_IRQHandler();

    EXPECT_TRUE(t1);
    EXPECT_TRUE(t11);
}

TEST_F(TimerIRQ_Test, Tim1CcVector_FiresTimer1Slot)
{
    bool t1 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_1, [&]() { t1 = true; }));

    TIM1_CC_IRQHandler();

    EXPECT_TRUE(t1);
}


/************************************************************************/
/* Per-vector routing -- shared TIM6/DAC and TIM8 vectors              */
/************************************************************************/
TEST_F(TimerIRQ_Test, Tim6DacVector_FiresTimer6AndDacUnderrun)
{
    bool t6 = false, dac = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_6, [&]() { t6 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::DAC_UNDERRUN, [&]() { dac = true; }));

    TIM6_DAC_IRQHandler();

    EXPECT_TRUE(t6);
    EXPECT_TRUE(dac);
}

TEST_F(TimerIRQ_Test, Tim8Brk_Tim12Vector_FiresTimer8AndTimer12)
{
    bool t8 = false, t12 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_8, [&]() { t8 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_12, [&]() { t12 = true; }));

    TIM8_BRK_TIM12_IRQHandler();

    EXPECT_TRUE(t8);
    EXPECT_TRUE(t12);
}

TEST_F(TimerIRQ_Test, Tim8Up_Tim13Vector_FiresTimer8AndTimer13)
{
    bool t8 = false, t13 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_8, [&]() { t8 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_13, [&]() { t13 = true; }));

    TIM8_UP_TIM13_IRQHandler();

    EXPECT_TRUE(t8);
    EXPECT_TRUE(t13);
}

TEST_F(TimerIRQ_Test, Tim8TrgCom_Tim14Vector_FiresTimer8AndTimer14)
{
    bool t8 = false, t14 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_8, [&]() { t8 = true; }));
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_14, [&]() { t14 = true; }));

    TIM8_TRG_COM_TIM14_IRQHandler();

    EXPECT_TRUE(t8);
    EXPECT_TRUE(t14);
}

TEST_F(TimerIRQ_Test, Tim8CcVector_FiresTimer8Slot)
{
    bool t8 = false;
    ASSERT_TRUE(TimerIRQ::Install(Slot::TIMER_8, [&]() { t8 = true; }));

    TIM8_CC_IRQHandler();

    EXPECT_TRUE(t8);
}


} // namespace
