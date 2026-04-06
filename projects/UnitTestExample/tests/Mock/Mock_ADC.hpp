#ifndef MOCK_ADC_HPP_
#define MOCK_ADC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IADC.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_ADC final : public IADC
{
public:
    Mock_ADC()
    {
        ON_CALL(*this, GetValue(_))
            .WillByDefault(Return(true));

        ON_CALL(*this, GetValueInterrupt(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(GetValue, bool(uint16_t& value));

    MOCK_METHOD1(GetValueInterrupt, bool(const std::function<void(uint16_t)>& handler));
};


#endif  // MOCK_ADC_HPP_
