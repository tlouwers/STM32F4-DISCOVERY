/**
 * \file    Button.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Button
 *
 * \brief   Onboard user button driver (B1, PA0) for the blink_orange sample application.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/blink_orange/Src/drivers
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef BUTTON_HPP_
#define BUTTON_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Button
{
public:
    static void Init();
    static bool IsPressed();
};


#endif  // BUTTON_HPP_
