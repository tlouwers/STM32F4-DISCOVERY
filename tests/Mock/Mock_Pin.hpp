#ifndef MOCK_PIN_HPP_
#define MOCK_PIN_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IPin.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <functional>


using ::testing::Return;
using ::testing::_;


class Mock_Pin final : public IPin
{
public:
    Mock_Pin()
    {
        ON_CALL(*this, Interrupt(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, InterruptEnable())
            .WillByDefault(Return(true));
        ON_CALL(*this, InterruptDisable())
            .WillByDefault(Return(true));
        ON_CALL(*this, InterruptRemove())
            .WillByDefault(Return(true));
        ON_CALL(*this, Get())
            .WillByDefault(Return(Level::LOW));
    }

    MOCK_METHOD3(Interrupt, bool(Trigger trigger, const std::function<void()>& callback, bool enabledAfterConfigure));
    MOCK_METHOD0(InterruptEnable, bool());
    MOCK_METHOD0(InterruptDisable, bool());
    MOCK_METHOD0(InterruptRemove, bool());

    MOCK_CONST_METHOD0(Toggle, void());
    MOCK_METHOD1(Set, void(Level level));
    MOCK_CONST_METHOD0(Get, Level());
};


#endif  // MOCK_PIN_HPP_
