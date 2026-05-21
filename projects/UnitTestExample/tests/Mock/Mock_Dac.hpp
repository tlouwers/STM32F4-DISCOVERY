/**
 * \file    Mock_Dac.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_Dac
 *
 * \brief   GMock implementation of the IDac interface for UnitTestExample.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/UnitTestExample/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_DAC_HPP_
#define MOCK_DAC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IDac.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_Dac final : public IDac
{
public:
    Mock_Dac()
    {
        ON_CALL(*this, SetValue(_, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, StartWaveform(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, StopWaveform(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD2(SetValue, bool(const Channel& channel, uint16_t length));

    MOCK_METHOD1(StartWaveform, bool(const Channel& channel));
    MOCK_METHOD1(StopWaveform, bool(const Channel& channel));
};


#endif  // MOCK_DAC_HPP_
