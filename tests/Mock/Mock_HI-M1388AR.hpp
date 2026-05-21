/**
 * \file    Mock_HI-M1388AR.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_HI_M1388AR
 *
 * \brief   GMock implementation of the IHI_M1388AR interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_HI_M1388AR_HPP_
#define MOCK_HI_M1388AR_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IHI_M1388AR.hpp"
#include "gmock/gmock.h"
#include <cstdint>


using ::testing::Return;
using ::testing::_;


class Mock_HI_M1388AR final : public IHI_M1388AR
{
public:
    Mock_HI_M1388AR()
    {
        ON_CALL(*this, Init(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, IsInit())
            .WillByDefault(Return(true));
        ON_CALL(*this, Sleep())
            .WillByDefault(Return(true));
        ON_CALL(*this, ClearDisplay())
            .WillByDefault(Return(true));
        ON_CALL(*this, WriteDigits(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(Init, bool(const IConfig& config));
    MOCK_CONST_METHOD0(IsInit, bool());
    MOCK_METHOD0(Sleep, bool());

    MOCK_METHOD0(ClearDisplay, bool());
    MOCK_METHOD1(WriteDigits, bool(const uint8_t* src));
};


#endif  // MOCK_HI_M1388AR_HPP_
