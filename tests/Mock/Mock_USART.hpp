#ifndef MOCK_USART_HPP_
#define MOCK_USART_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IUSART.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_USART final : public IUSART
{
public:
    Mock_USART()
    {
        ON_CALL(*this, WriteDMA(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadDMA(_, _, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteInterrupt(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadInterrupt(_, _, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteBlocking(_, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadBlocking(_, _))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD3(WriteDMA, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(ReadDMA, bool(uint8_t* dest, uint16_t length, const std::function<void(uint16_t)>& handler, bool useIdleDetection = true));

    MOCK_METHOD3(WriteInterrupt, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(ReadInterrupt, bool(uint8_t* dest, uint16_t length, const std::function<void(uint16_t)>& handler, bool useIdleDetection = true);

    MOCK_METHOD2(WriteBlocking, bool(const uint8_t* src, uint16_t length));
    MOCK_METHOD2(ReadBlocking, bool(uint8_t* dest, uint16_t length));
};


#endif  // MOCK_SPI_HPP_
