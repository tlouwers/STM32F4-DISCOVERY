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
 * \brief   Unit tests for the FreeRTOSProject AppLogic class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/FreeRTOSProject/tests
 *
 * \details Exercises the interface-injected application logic with Mock
 *          drivers (Mock_LIS3DSH, Mock_Pin). Application itself is the
 *          composition root (FreeRTOS task/notify plumbing) and is
 *          intentionally not tested.
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


using ::testing::_;
using ::testing::Return;


/************************************************************************/
/* Test fixture                                                         */
/************************************************************************/
class AppLogic_Test : public ::testing::Test
{
protected:
    Mock_LIS3DSH mAccelerometer;
    Mock_Pin     mMotionLed;

    AppLogic mSubject { mAccelerometer, mMotionLed };
};


/************************************************************************/
/* Test cases                                                           */
/************************************************************************/
TEST_F(AppLogic_Test, OnMotionData_ThenProcess_ReadsAccelerometerWithReportedLength)
{
    EXPECT_CALL(mMotionLed, Toggle()).Times(::testing::AnyNumber());

    mSubject.OnMotionData(25);

    EXPECT_CALL(mAccelerometer, RetrieveAxesData(_, 25)).WillOnce(Return(true));

    mSubject.ProcessMotionData();
}

TEST_F(AppLogic_Test, ProcessMotionData_WithMotionData_TogglesMotionLed)
{
    mSubject.OnMotionData(7);

    EXPECT_CALL(mAccelerometer, RetrieveAxesData(_, _)).Times(::testing::AnyNumber()).WillRepeatedly(Return(true));
    EXPECT_CALL(mMotionLed, Toggle()).Times(1);

    mSubject.ProcessMotionData();
}

TEST_F(AppLogic_Test, ProcessMotionData_WithoutMotionData_DoesNotReadAccelerometer)
{
    // Misbehaving-mock guard: with no reported samples the length gate must
    // suppress the read entirely. If the gate were wrong the accelerometer
    // would be read (and the LED toggled), breaking these Times(0).
    EXPECT_CALL(mAccelerometer, RetrieveAxesData(_, _)).Times(0);
    EXPECT_CALL(mMotionLed, Toggle()).Times(0);

    mSubject.ProcessMotionData();
}
