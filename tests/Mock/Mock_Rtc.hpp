#ifndef MOCK_RTC_HPP_
#define MOCK_RTC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Interfaces/IRtc.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_Rtc final : public IRtc
{
public:
    Mock_Rtc()
    {
        ON_CALL(*this, SetDateTime(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, GetDateTime(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(SetDateTime, bool(const DateTime& dateTime));
    MOCK_METHOD1(GetDateTime, bool(DateTime& dateTime));
};


#endif  // MOCK_RTC_HPP_
