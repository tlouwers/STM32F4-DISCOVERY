/**
 * \file    Mock_Adc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_Adc
 *
 * \brief   GMock implementation of the IAdc interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_ADC_HPP_
#define MOCK_ADC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IAdc.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_Adc final : public IAdc
{
public:
    Mock_Adc()
    {
        ON_CALL(*this, GetValue(_))
            .WillByDefault(Return(true));

        ON_CALL(*this, GetValueInterrupt(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(GetValue, bool(uint16_t& value));

    MOCK_METHOD1(GetValueInterrupt, bool(const std::function<void(uint16_t)>& handler));
};


#endif  // MOCK_ADC_HPP_
