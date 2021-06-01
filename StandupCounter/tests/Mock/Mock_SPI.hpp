#ifndef MOCK_SPI_HPP_
#define MOCK_SPI_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Interfaces/ISPI.hpp"
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
        ON_CALL(*this, ReadBlocking(_, _))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD3(WriteDMA, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(WriteReadDMA, bool(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD3(ReadDMA, bool(uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD3(WriteInterrupt, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(WriteReadInterrupt, bool (const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD3(ReadInterrupt, bool(uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD2(WriteBlocking, bool(const uint8_t* src, uint16_t length));
    MOCK_METHOD3(WriteReadBlocking, bool(const uint8_t* src, uint8_t* dest, uint16_t length));
    MOCK_METHOD2(ReadBlocking, bool(uint8_t* dest, uint16_t length));
};


#endif  // MOCK_SPI_HPP_
