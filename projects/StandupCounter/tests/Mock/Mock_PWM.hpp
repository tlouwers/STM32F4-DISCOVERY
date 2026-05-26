/**
 * \file    Mock_PWM.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_PWM
 *
 * \brief   GMock implementation of the IPWM interface for StandupCounter.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/StandupCounter/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_PWM_HPP_
#define MOCK_PWM_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IPWM.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_PWM final : public IPWM
{
public:
    Mock_PWM()
    {
        ON_CALL(*this, Start(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, Stop(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(Start, bool(Channel channel));
    MOCK_METHOD1(Stop, bool(Channel channel));
};


#endif  // MOCK_PWM_HPP_
