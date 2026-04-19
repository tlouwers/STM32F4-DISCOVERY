#ifndef MOCK_DAC_HPP_
#define MOCK_DAC_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IDac.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::_;


class Mock_Dac final : public IDac
{
public:
    Mock_Dac()
    {
        ON_CALL(*this, SetValue(_, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, StartWaveform(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, StopWaveform(_))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD2(SetValue, bool(const Channel& channel, uint16_t length));

    MOCK_METHOD1(StartWaveform, bool(const Channel& channel));
    MOCK_METHOD1(StopWaveform, bool(const Channel& channel));
};


#endif  // MOCK_DAC_HPP_
