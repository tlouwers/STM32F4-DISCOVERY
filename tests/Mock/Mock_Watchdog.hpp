/**
 * \file     Mock_Watchdog.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_Watchdog
 *
 * \brief   GMock implementation of the IWatchdog interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_WATCHDOG_HPP_
#define MOCK_WATCHDOG_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IWatchdog.hpp"
#include "gmock/gmock.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Mock_Watchdog final : public IWatchdog
{
public:
    MOCK_CONST_METHOD0(Refresh, void());
};


#endif  // MOCK_WATCHDOG_HPP_
