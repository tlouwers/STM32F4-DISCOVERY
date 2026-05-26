/**
 * \file    Mock_SPI.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_SPI
 *
 * \brief   GMock implementation of the ISPI interface for ExampleProject.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/ExampleProject/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_SPI_HPP_
#define MOCK_SPI_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/ISPI.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_SPI final : public ISPI
{
public:
    enum class Response : uint8_t
    {
        SelfTest
    };

    Mock_SPI()
    {
        ON_CALL(*this, WriteDMA(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, WriteReadDMA(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadDMA(_, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteInterrupt(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, WriteReadInterrupt(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadInterrupt(_, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteBlocking(_, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, WriteReadBlocking(_, _, _))
            .WillByDefault(Return(true));
        //ON_CALL(*this, ReadBlocking(_, _))
        //    .WillByDefault(Return(true));
    }

    MOCK_METHOD3(WriteDMA, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(WriteReadDMA, bool(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD3(ReadDMA, bool(uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD3(WriteInterrupt, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(WriteReadInterrupt, bool (const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD3(ReadInterrupt, bool(uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD2(WriteBlocking, bool(const uint8_t* src, uint16_t length));
    MOCK_METHOD3(WriteReadBlocking, bool(const uint8_t* src, uint8_t* dest, uint16_t length));
    //MOCK_METHOD2(ReadBlocking, bool(uint8_t* dest, uint16_t length));
    bool ReadBlocking(uint8_t* dest, uint16_t length)
    {
        if (dest == nullptr) { return false; }
        if (length == 0)     { return false; }

        switch (response)
        {
            case Response::SelfTest:
                if (length > sizeof(who_am_i)) { return false; }

                if (length == 1)
                {
                    std::memcpy(dest, who_am_i, length);
                }
                break;
            default:
                return false;
                break;
        }

        return true;
    }

    // Helper method to set a response from SPI (ReadBlocking).
    void SetResponse(Response desired_response)
    {
        response = desired_response;
    }

private:
    Response response = Response::SelfTest;
    const uint8_t who_am_i[1] = { 0x3F };
};


#endif  // MOCK_SPI_HPP_
