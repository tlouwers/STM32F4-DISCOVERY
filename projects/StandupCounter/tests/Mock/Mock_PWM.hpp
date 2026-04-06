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
