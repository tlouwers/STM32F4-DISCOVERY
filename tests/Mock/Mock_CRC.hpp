#ifndef MOCK_CRC_HPP_
#define MOCK_CRC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/ICRC.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_CRC final : public ICRC
{
public:
    Mock_CRC()
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
