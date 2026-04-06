#include "gtest/gtest.h"


// Test subject
#include "Fake/drivers/Crc/CRC.hpp"

// Supporting files
#include "board/BoardConfig.hpp"


namespace {


// Constants
static constexpr uint32_t buffer[6] = { 0x01234567, 0x12345678, 0x23456789, 0x34567890, 0x45678901, 0x56789012 };


// Test fixture for CRC.
class CRC_Test : public ::testing::Test
{
protected:
    CRC_Test() :
        mSubject()
    {
        // Initialize test matter
    }

    Crc mSubject;
};


TEST_F(CRC_Test, Calculate_buffer_nullptr)
{
    EXPECT_EQ(0, mSubject.Calculate(nullptr, 1));
}

TEST_F(CRC_Test, Calculate_length_null)
{
    EXPECT_EQ(0, mSubject.Calculate(const_cast<uint32_t*>(buffer), 0));
}

TEST_F(CRC_Test, Calculate)
{
    EXPECT_EQ(0x63EC482A, mSubject.Calculate(const_cast<uint32_t*>(buffer), 6));
}


} // namespace
