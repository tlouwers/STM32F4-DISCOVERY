/**
 * \file    Mock_Crc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_Crc
 *
 * \brief   GMock implementation of the ICrc interface for UnitTestExample.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/UnitTestExample/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_CRC_HPP_
#define MOCK_CRC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/ICrc.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_Crc final : public ICrc
{
public:
    Mock_Crc()
    {
        ON_CALL(*this, Calculate(_,_))
            .WillByDefault(Return(0));
    }

    MOCK_METHOD2(Calculate, uint32_t(uint32_t* buffer, uint32_t length));
/*
    uint32_t Calculate(uint32_t* buffer, uint32_t length)
    {
        constexpr uint32_t reference[6] = { 0x01234567, 0x12345678, 0x23456789, 0x34567890, 0x45678901, 0x56789012 };

        if (buffer == nullptr) { return 0; }
        if (length == 0)       { return 0; }

        if (length == 6)
        {
            bool result = true;
            for (auto i = 0; i < length; i++)
            {
                if (buffer[i] != reference[i])
                {
                    result = false;
                }
            }

            if (result == true)
            {
                return 0x63EC482A;
            }
        }
        return 0;
    }
*/
};


#endif  // MOCK_CRC_HPP_
