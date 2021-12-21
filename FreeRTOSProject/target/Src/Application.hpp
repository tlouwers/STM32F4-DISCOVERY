/**
 * \file     Application.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for FreeRTOS demo.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/FreeRTOSProject/target/Src
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    12-2021
 */

#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include "config.h"
#include "drivers/Pin/Pin.hpp"


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
    void Error();

    bool CreateTasks();
    void StartTasks();

private:
    Pin mButton;
    Pin mLedGreen;
    Pin mLedOrange;
    Pin mLedRed;
    Pin mLedBlue;

    std::atomic<bool> mShouldBlinkLeds;

    void CallbackButtonPressed();

    void CallbackLedGreenToggle();
    void CallbackLedOrangeToggle();
    void CallbackLedRedToggle();
    void CallbackLedBlueToggle();
};


#endif  // APPLICATION_HPP_
