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
 * \brief   Unit tests for the TiltExample AppLogic class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/TiltExample/tests
 *
 * \details Exercises the interface-injected application logic with Mock
 *          drivers (Mock_LIS3DSH, Mock_Pin, Mock_HI_M1388AR), including the
 *          pure motion-to-pitch/roll conversion and pixel mapping.
 *          Application itself is the composition root (FreeRTOS task/queue
 *          plumbing and USART output) and is intentionally not tested.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "AppLogic.hpp"
#include "Mock_LIS3DSH.hpp"
#include "Mock_Pin.hpp"
#include "Mock_HI-M1388AR.hpp"


using ::testing::_;
using ::testing::Return;


/************************************************************************/
/* Test fixture                                                         */
/************************************************************************/
class AppLogic_Test : public ::testing::Test
{
protected:
    Mock_LIS3DSH    mAccelerometer;
    Mock_Pin        mMotionLed;
    Mock_HI_M1388AR mMatrix;

    AppLogic mSubject { mAccelerometer, mMotionLed, mMatrix };
};


/************************************************************************/
/* Test cases                                                           */
/************************************************************************/
TEST_F(AppLogic_Test, CalculateMotionSample_FlatOnTable_PitchAndRollZero)
{
    MotionSampleRaw raw { 0, 0, 16384 };

    MotionSample sample = mSubject.CalculateMotionSample(raw);

    EXPECT_FLOAT_EQ(0.0f, sample.pitch);
    EXPECT_FLOAT_EQ(0.0f, sample.roll);
    EXPECT_GT(sample.Z, 0.0f);
}

TEST_F(AppLogic_Test, CalculateMotionSample_FortyFiveDegreePitch_ReportedAsFortyFive)
{
    // Equal positive Y and Z components -> atan2(Y,Z) == 45 degrees.
    MotionSampleRaw raw { 0, 10000, 10000 };

    MotionSample sample = mSubject.CalculateMotionSample(raw);

    EXPECT_NEAR(45.0f, sample.pitch, 0.01f);
    EXPECT_FLOAT_EQ(0.0f, sample.roll);
}

TEST_F(AppLogic_Test, CalculatePixel_CenteredSample_LightsCentrePixel)
{
    MotionSample sample {};
    sample.pitch = 0.0f;
    sample.roll  = 0.0f;

    uint8_t pixels[8] = {};
    mSubject.CalculatePixel(pixels, sample, false);

    EXPECT_EQ(0x10, pixels[4]);     // columnPitch 4 -> (1 << 4); rowRoll 4 -> dest[4]
    for (int i = 0; i < 8; ++i)
    {
        if (i != 4) { EXPECT_EQ(0x00, pixels[i]); }
    }
}

TEST_F(AppLogic_Test, CalculatePixel_Inverted_ReversesTheRowOrder)
{
    MotionSample sample {};
    sample.pitch = 0.0f;
    sample.roll  = 0.0f;

    uint8_t pixels[8] = {};
    mSubject.CalculatePixel(pixels, sample, true);

    EXPECT_EQ(0x10, pixels[3]);     // row 4 reversed over 8 entries -> index 7 - 4 == 3
    for (int i = 0; i < 8; ++i)
    {
        if (i != 3) { EXPECT_EQ(0x00, pixels[i]); }
    }
}

TEST_F(AppLogic_Test, RetrieveMotion_AfterOnMotionData_ReadsAccelerometerAndTogglesLed)
{
    mSubject.OnMotionData(6);

    EXPECT_CALL(mMotionLed, Toggle()).Times(1);
    EXPECT_CALL(mAccelerometer, RetrieveAxesData(_, 6)).WillOnce(Return(true));

    uint8_t buffer[6] = {};
    uint8_t length = 0;
    EXPECT_TRUE(mSubject.RetrieveMotion(buffer, length));
    EXPECT_EQ(6, length);
}

TEST_F(AppLogic_Test, RetrieveMotion_WithoutMotionData_DoesNotReadOrToggle)
{
    // Misbehaving-mock guard: with no pending samples the length gate must
    // suppress the read and the LED toggle entirely. A broken gate would
    // call into the accelerometer / pin, breaking these Times(0).
    EXPECT_CALL(mAccelerometer, RetrieveAxesData(_, _)).Times(0);
    EXPECT_CALL(mMotionLed, Toggle()).Times(0);

    uint8_t buffer[6] = {};
    uint8_t length = 1;
    EXPECT_FALSE(mSubject.RetrieveMotion(buffer, length));
    EXPECT_EQ(0, length);
}

TEST_F(AppLogic_Test, RenderTilt_WritesToTheMatrix)
{
    MotionSample sample {};
    sample.pitch = 0.0f;
    sample.roll  = 0.0f;

    EXPECT_CALL(mMatrix, WriteDigits(_)).Times(1).WillOnce(Return(true));

    mSubject.RenderTilt(sample);
}
