/**
 * \file    TestCrc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the fake CRC driver's reference outputs,
 *          used to validate test wiring rather than the real CRC unit.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "gtest/gtest.h"


// Test subject
#include "Fake/drivers/Crc/Crc.hpp"

// Supporting files
#include "board/BoardConfig.hpp"


namespace {


// Constants
static constexpr uint32_t buffer[6] = { 0x01234567, 0x12345678, 0x23456789, 0x34567890, 0x45678901, 0x56789012 };


// Test fixture for Crc.
class Crc_Test : public ::testing::Test
{
protected:
    Crc_Test() :
        mSubject()
    {
        // Initialize test matter
    }

    Crc mSubject;
};


TEST_F(Crc_Test, Calculate_buffer_nullptr)
{
    uint32_t out = 0xDEADBEEFu;
    EXPECT_FALSE(mSubject.Calculate(nullptr, 1, out));
    EXPECT_EQ(0xDEADBEEFu, out);    // out left untouched on failure
}

TEST_F(Crc_Test, Calculate_length_null)
{
    uint32_t out = 0xDEADBEEFu;
    EXPECT_FALSE(mSubject.Calculate(buffer, 0, out));
    EXPECT_EQ(0xDEADBEEFu, out);
}

TEST_F(Crc_Test, Calculate)
{
    uint32_t out = 0;
    EXPECT_TRUE(mSubject.Calculate(buffer, 6, out));
    EXPECT_EQ(0x63EC482Au, out);
}


} // namespace
