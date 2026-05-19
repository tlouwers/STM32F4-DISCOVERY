/**
 * \file    AppLogic.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Interface-injected application logic for the FreeRTOS demo.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/FreeRTOSProject/target/Src
 *
 * \details Holds the testable behaviour split out of Application: the motion
 *          data path.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "AppLogic.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, binds the injected driver interfaces.
 * \param   accelerometer   Accelerometer used to retrieve motion samples.
 * \param   motionLed       Led toggled when motion data is processed.
 */
AppLogic::AppLogic(ILIS3DSH& accelerometer, IPin& motionLed) :
    mAccelerometer(accelerometer),
    mMotionLed(motionLed),
    mMotionLength(0)
{ ; }

/**
 * \brief   Handler for the motion-data-received event. Records the reported
 *          sample length so the motion task can drain it later.
 * \param   length  The number of samples reported by the accelerometer.
 * \note    Called from ISR context in the composition root.
 */
void AppLogic::OnMotionData(uint8_t length)
{
    mMotionLength = length;
}

/**
 * \brief   Drains pending motion data (if any) by reading the accelerometer
 *          samples. Driven by the motion FreeRTOS task in the composition root.
 */
void AppLogic::ProcessMotionData()
{
    if (mMotionLength > 0)
    {
        mMotionLed.Toggle();

        bool retrieveResult = mAccelerometer.RetrieveAxesData(mMotionArray, mMotionLength);
        EXPECT(retrieveResult);
        (void)(retrieveResult);

        // Deinterleave to X,Y,Z samples
    }
}
