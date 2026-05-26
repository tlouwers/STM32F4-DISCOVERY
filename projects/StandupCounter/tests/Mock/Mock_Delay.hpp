/**
 * \file    Mock_Delay.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_Delay
 *
 * \brief   GMock implementation of the IDelay interface for StandupCounter.
 *
 * \details A recording delay: DelayMs() simply records that a wait of the
 *          given duration was requested and returns immediately, so a 105 s
 *          firmware wait costs zero real time in the test suite. Tests assert
 *          the requested durations via EXPECT_CALL.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/StandupCounter/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_DELAY_HPP_
#define MOCK_DELAY_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/IDelay.hpp"
#include "gmock/gmock.h"
#include <cstdint>


using ::testing::_;


class Mock_Delay final : public IDelay
{
public:
    MOCK_METHOD1(DelayMs, void(uint32_t ms));
};


#endif  // MOCK_DELAY_HPP_
