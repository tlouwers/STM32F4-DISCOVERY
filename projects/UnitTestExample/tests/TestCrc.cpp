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
    uint32_t out = 0;
    EXPECT_FALSE(mSubject.Calculate(nullptr, 1, out));
}

TEST_F(Crc_Test, Calculate_length_null)
{
    uint32_t out = 0;
    EXPECT_FALSE(mSubject.Calculate(buffer, 0, out));
}

TEST_F(Crc_Test, Calculate)
{
    uint32_t out = 0;
    EXPECT_TRUE(mSubject.Calculate(buffer, 6, out));
    EXPECT_EQ(0x63EC482A, out);
}


} // namespace
