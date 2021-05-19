/**
 * \file    main.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main entry point for Accelerometer demo.
 *
* \note     https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/ExampleProject/target/Src
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    10-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "stm32f4xx_hal.h"
#include "board/Board.hpp"
#include "Application.hpp"


/************************************************************************/
/* Main application entry point                                         */
/************************************************************************/
/**
 * \brief       Main application entry point.
 * \returns     Never returns.
 */
int main()
{
    Board::InitPins();
    Board::InitClock();     // ignore result?


    HAL_Init();


    Application mApp;

    if (!mApp.Init())
    {
        mApp.Error();
    }

    while (1)
    {
        mApp.Process();
    }
}
