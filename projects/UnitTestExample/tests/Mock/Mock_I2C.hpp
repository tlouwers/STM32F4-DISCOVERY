/**
 * \file    Mock_I2C.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_I2C
 *
 * \brief   GMock implementation of the II2C interface for UnitTestExample.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/UnitTestExample/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_I2C_HPP_
#define MOCK_I2C_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/II2C.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_I2C final : public II2C
{
public:
    Mock_I2C()
    {
        ON_CALL(*this, WriteDMA(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadDMA(_, _, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteInterrupt(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadInterrupt(_, _, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteBlocking(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadBlocking(_, _, _))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD4(WriteDMA, bool(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(ReadDMA, bool(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD4(WriteInterrupt, bool(uint8_t slave, const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(ReadInterrupt, bool(uint8_t slave, uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD3(WriteBlocking, bool(uint8_t slave, const uint8_t* src, uint16_t length));
    MOCK_METHOD3(ReadBlocking, bool(uint8_t slave, uint8_t* dest, uint16_t length));
};


#endif  // MOCK_I2C_HPP_
