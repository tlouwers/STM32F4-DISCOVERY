/**
 * \file     AppLogic.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   AppLogic
 *
 * \brief   Interface-injected application logic for the FreeRTOS demo.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/FreeRTOSProject/target/Src
 *
 * \details Holds the testable behaviour split out of Application: the motion
 *          data path. It depends only on driver interfaces (ILIS3DSH, IPin)
 *          so it can be unit-tested with Mock drivers, while Application
 *          remains the thin composition root that constructs and wires the
 *          concrete drivers and owns the FreeRTOS task/notify plumbing.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef APPLOGIC_HPP_
#define APPLOGIC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include <cstdint>
#include "interfaces/ILIS3DSH.hpp"
#include "interfaces/IPin.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Testable application logic, injected with driver interfaces.
 */
class AppLogic final
{
public:
    AppLogic(ILIS3DSH& accelerometer, IPin& motionLed);

    void OnMotionData(uint8_t length);
    void ProcessMotionData();

private:
    ILIS3DSH& mAccelerometer;
    IPin&     mMotionLed;

    std::atomic<uint8_t> mMotionLength;
    uint8_t              mMotionArray[25 * 3 * 2] = {};
};


#endif  // APPLOGIC_HPP_
