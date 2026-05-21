/**
 * \file    Mock_Rtc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_Rtc
 *
 * \brief   GMock implementation of the IRtc interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_RTC_HPP_
#define MOCK_RTC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Interfaces/IRtc.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_Rtc final : public IRtc
{
public:
    Mock_Rtc()
    {
        ON_CALL(*this, SetDateTime(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, GetDateTime(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(SetDateTime, bool(const DateTime& dateTime));
    MOCK_METHOD1(GetDateTime, bool(DateTime& dateTime));
};


#endif  // MOCK_RTC_HPP_
