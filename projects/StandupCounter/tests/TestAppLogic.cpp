/**
 * \file    TestAppLogic.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Unit tests for the StandupCounter AppLogic class.
 *
 * \details Exercises the interface-injected standup countdown choreography
 *          with Mock drivers (Mock_HI_M1388AR, Mock_PWM, Mock_Pin) and a
 *          recording delay mock (Mock_Delay) so the 105 s speaking wait is
 *          asserted without ever costing real time. Application itself is the
 *          composition root and is intentionally not tested.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/StandupCounter/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstring>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "AppLogic.hpp"
#include "Mock_HI_M1388AR.hpp"
#include "Mock_PWM.hpp"
#include "Mock_Delay.hpp"
#include "Mock_Pin.hpp"
#include "components/HI-M1388AR/HI-M1388AR_Lib.hpp"


using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::Mock;


/************************************************************************/
/* Matchers                                                             */
/************************************************************************/
// constexpr glyph arrays have internal linkage, so each translation unit has
// its own copy: the pointer AppLogic.cpp passes differs from this TU's array.
// Match on the 8-byte glyph CONTENT instead of pointer identity.
MATCHER_P(DigitEq, expected, "matches the 8x8 matrix glyph by content")
{
    return std::memcmp(arg, expected, MATRIX_SIZE) == 0;
}


/************************************************************************/
/* Test fixture                                                         */
/************************************************************************/
class AppLogic_Test : public ::testing::Test
{
protected:
    Mock_HI_M1388AR mMatrix;
    Mock_PWM        mPWM;
    Mock_Pin        mLedGreen;
    Mock_Pin        mLedOrange;
    Mock_Pin        mLedRed;
    Mock_Delay      mClock;

    AppLogic mSubject { mMatrix, mPWM, mLedGreen, mLedOrange, mLedRed, mClock };

    // The mocks are 'final', so NiceMock cannot wrap them; absorb the expected
    // hardware chatter with AnyNumber catch-alls instead. Specific expectations
    // declared in a test after this call take priority for the calls they match.
    void AllowHardwareChatter()
    {
        EXPECT_CALL(mMatrix, WriteDigits(_)).Times(AnyNumber());
        EXPECT_CALL(mMatrix, ClearDisplay()).Times(AnyNumber());
        EXPECT_CALL(mPWM, Start(_)).Times(AnyNumber());
        EXPECT_CALL(mPWM, Stop(_)).Times(AnyNumber());
        EXPECT_CALL(mLedGreen, Set(_)).Times(AnyNumber());
        EXPECT_CALL(mLedOrange, Set(_)).Times(AnyNumber());
        EXPECT_CALL(mLedRed, Set(_)).Times(AnyNumber());
        EXPECT_CALL(mClock, DelayMs(_)).Times(AnyNumber());
    }
};


/************************************************************************/
/* Test cases                                                           */
/************************************************************************/
TEST_F(AppLogic_Test, Process_WithoutButtonPress_DoesNothing)
{
    EXPECT_CALL(mMatrix, WriteDigits(_)).Times(0);
    EXPECT_CALL(mPWM, Start(_)).Times(0);
    EXPECT_CALL(mClock, DelayMs(_)).Times(0);

    mSubject.Process();
}

TEST_F(AppLogic_Test, Process_AfterButtonPress_RunsFullCountdownSequence)
{
    EXPECT_CALL(mLedGreen, Set(_)).Times(AnyNumber());
    EXPECT_CALL(mLedOrange, Set(_)).Times(AnyNumber());
    EXPECT_CALL(mLedRed, Set(_)).Times(AnyNumber());
    EXPECT_CALL(mClock, DelayMs(_)).Times(AnyNumber());

    // smiley + 10 countdown digits + sadface = 12 writes; 10 short beeps + 1
    // long beep = 11 Start/Stop pairs; the display is cleared once at the end.
    EXPECT_CALL(mMatrix, WriteDigits(_)).Times(12);
    EXPECT_CALL(mMatrix, ClearDisplay()).Times(1);
    EXPECT_CALL(mPWM, Start(IPWM::Channel::CHANNEL_1)).Times(11);
    EXPECT_CALL(mPWM, Stop(IPWM::Channel::CHANNEL_1)).Times(11);

    mSubject.OnButtonPressed();
    mSubject.Process();
}

TEST_F(AppLogic_Test, Process_AfterButtonPress_RequestsLongSpeakingWaitWithoutRealDelay)
{
    AllowHardwareChatter();

    // The 105 s "person is speaking" wait is requested, but the recording mock
    // returns immediately, so the test never actually sleeps.
    EXPECT_CALL(mClock, DelayMs(105000u)).Times(1);

    mSubject.OnButtonPressed();
    mSubject.Process();
}

TEST_F(AppLogic_Test, Process_AfterButtonPress_DisplaysDigitsNineDownToZeroInOrder)
{
    EXPECT_CALL(mMatrix, ClearDisplay()).Times(AnyNumber());
    EXPECT_CALL(mPWM, Start(_)).Times(AnyNumber());
    EXPECT_CALL(mPWM, Stop(_)).Times(AnyNumber());
    EXPECT_CALL(mLedGreen, Set(_)).Times(AnyNumber());
    EXPECT_CALL(mLedOrange, Set(_)).Times(AnyNumber());
    EXPECT_CALL(mLedRed, Set(_)).Times(AnyNumber());
    EXPECT_CALL(mClock, DelayMs(_)).Times(AnyNumber());

    InSequence seq;
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(symbol_smiley)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_nine)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_eight)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_seven)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_six)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_five)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_four)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_three)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_two)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_one)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(digit_zero)));
    EXPECT_CALL(mMatrix, WriteDigits(DigitEq(symbol_sadface)));

    mSubject.OnButtonPressed();
    mSubject.Process();
}

TEST_F(AppLogic_Test, Process_CalledTwiceAfterSinglePress_RunsOnlyOnce)
{
    // Phase 1: allow and consume the single pending press.
    AllowHardwareChatter();
    mSubject.OnButtonPressed();
    mSubject.Process();

    Mock::VerifyAndClearExpectations(&mMatrix);
    Mock::VerifyAndClearExpectations(&mPWM);
    Mock::VerifyAndClearExpectations(&mClock);

    // Phase 2 — misbehaving-mock guard: if AppLogic failed to clear its pending
    // flag, the sequence would run again here, breaking these Times(0) gates.
    EXPECT_CALL(mMatrix, WriteDigits(_)).Times(0);
    EXPECT_CALL(mPWM, Start(_)).Times(0);
    EXPECT_CALL(mClock, DelayMs(_)).Times(0);

    mSubject.Process();
}
