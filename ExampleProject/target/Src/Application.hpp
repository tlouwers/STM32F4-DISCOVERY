/**
 * \file Application.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for Accelerometer demo.
 *
 * \note    <ToDo: link to url on github>
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        10-2019
 */

#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include "drivers/Pin/Pin.hpp"
#include "utility/CpuWakeCounter.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Main application class.
 */
class Application
{
public:
    Application();
    virtual ~Application() {};

    bool Init();
    void Process();
    void Error();

private:
    Pin mButton;
    Pin mLedGreen;
    Pin mLedOrange;
    Pin mLedRed;
    Pin mLedBlue;
    CpuWakeCounter mCpuWakeCounter;

    std::atomic<bool> mButtonPressed;

    void ButtonPressedCallback();

    void GetUsedStack();
    void GetUsedHeap();
};


#endif  // APPLICATION_HPP_
