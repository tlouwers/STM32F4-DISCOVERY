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
