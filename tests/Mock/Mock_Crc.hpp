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
 * \brief   GMock implementation of the ICrc interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Mock
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
        ON_CALL(*this, Calculate(_,_,_))
            .WillByDefault(Return(false));
    }

    MOCK_METHOD3(Calculate, bool(const uint32_t* buffer, uint32_t length, uint32_t& out));
};


#endif  // MOCK_CRC_HPP_
