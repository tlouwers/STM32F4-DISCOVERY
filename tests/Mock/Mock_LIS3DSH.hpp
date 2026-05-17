#ifndef MOCK_LIS3DSH_HPP_
#define MOCK_LIS3DSH_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/ILIS3DSH.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <functional>


using ::testing::Return;
using ::testing::_;


class Mock_LIS3DSH final : public ILIS3DSH
{
public:
    Mock_LIS3DSH()
    {
        ON_CALL(*this, Init(_))
            .WillByDefault(Return(true));
        ON_CALL(*this, IsInit())
            .WillByDefault(Return(true));
        ON_CALL(*this, Sleep())
            .WillByDefault(Return(true));
        ON_CALL(*this, Enable())
            .WillByDefault(Return(true));
        ON_CALL(*this, Disable())
            .WillByDefault(Return(true));
        ON_CALL(*this, RetrieveAxesData(_, _))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD1(Init, bool(const IConfig& config));
    MOCK_CONST_METHOD0(IsInit, bool());
    MOCK_METHOD0(Sleep, bool());

    MOCK_METHOD0(Enable, bool());
    MOCK_METHOD0(Disable, bool());
    MOCK_METHOD1(SetHandler, void(const std::function<void(uint8_t length)>& handler));
    MOCK_METHOD2(RetrieveAxesData, bool(uint8_t* dest, uint8_t length));
};


#endif  // MOCK_LIS3DSH_HPP_
