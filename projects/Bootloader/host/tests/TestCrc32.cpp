#include "gtest/gtest.h"
#include "crc/SoftwareCrc32.hpp"


class SoftwareCrc32_Test : public ::testing::Test
{
protected:
    SoftwareCrc32 crc;
};

// ── Known-answer tests ──────────────────────────────────────────────────────
// Reference values generated with the STM32F4 hardware CRC peripheral
// (CRC-32/MPEG-2: poly 0x04C11DB7, init 0xFFFFFFFF, no reflection, no final XOR).

TEST_F(SoftwareCrc32_Test, SingleWord_0x00000000)
{
    // STM32 CRC of a single 32-bit word 0x00000000
    const uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };
    EXPECT_EQ(crc.Compute(data, sizeof(data)), 0xC704DD7Bu);
}

TEST_F(SoftwareCrc32_Test, SingleWord_0xFFFFFFFF)
{
    // CRC(0xFFFFFFFF ^ 0xFFFFFFFF = 0x00000000, then process) = 0x00000000
    const uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    EXPECT_EQ(crc.Compute(data, sizeof(data)), 0x00000000u);
}

TEST_F(SoftwareCrc32_Test, SingleWord_0x12345678)
{
    // Known STM32 HW CRC result for single word 0x12345678
    const uint8_t data[] = { 0x12, 0x34, 0x56, 0x78 };
    EXPECT_EQ(crc.Compute(data, sizeof(data)), 0xDF8A8A2Bu);
}

TEST_F(SoftwareCrc32_Test, TwoWords)
{
    // Two 32-bit words: 0x00000001, 0x00000002
    const uint8_t data[] = {
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x02
    };
    EXPECT_EQ(crc.Compute(data, sizeof(data)), 0x298BE7BAu);
}

TEST_F(SoftwareCrc32_Test, AscendingBytes_1_to_8)
{
    // Two words: 0x01020304, 0x05060708
    const uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    EXPECT_EQ(crc.Compute(data, sizeof(data)), 0x140B8DD8u);
}

TEST_F(SoftwareCrc32_Test, NullPointer_ZeroLength)
{
    // Edge case: zero-length input should return init value 0xFFFFFFFF
    EXPECT_EQ(crc.Compute(nullptr, 0), 0xFFFFFFFFu);
}

TEST_F(SoftwareCrc32_Test, ComputeTwice_SameResult)
{
    // Verify the CRC object is stateless between calls
    const uint8_t data[] = { 0x12, 0x34, 0x56, 0x78 };
    uint32_t first  = crc.Compute(data, sizeof(data));
    uint32_t second = crc.Compute(data, sizeof(data));
    EXPECT_EQ(first, second);
}

TEST_F(SoftwareCrc32_Test, LargerBuffer_16Bytes)
{
    // Four words: 0xDEADBEEF, 0xCAFEBABE, 0x8BADF00D, 0xFEEDFACE
    const uint8_t data[] = {
        0xDE, 0xAD, 0xBE, 0xEF,
        0xCA, 0xFE, 0xBA, 0xBE,
        0x8B, 0xAD, 0xF0, 0x0D,
        0xFE, 0xED, 0xFA, 0xCE
    };
    EXPECT_EQ(crc.Compute(data, sizeof(data)), 0xF1B36814u);
}
