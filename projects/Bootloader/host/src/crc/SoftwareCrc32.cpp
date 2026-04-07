/**
 * \file    SoftwareCrc32.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SoftwareCrc32
 *
 * \brief   Software CRC-32 implementation matching the STM32F4 hardware CRC peripheral.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/crc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "crc/SoftwareCrc32.hpp"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static constexpr uint32_t kPolynomial = 0x04C11DB7u;
static constexpr uint32_t kInitValue  = 0xFFFFFFFFu;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, builds the CRC lookup table.
 */
SoftwareCrc32::SoftwareCrc32()
{
    BuildTable();
}

/**
 * \brief   Computes the CRC-32/MPEG-2 over the given byte buffer.
 * \param   data    Pointer to the first byte of the buffer.
 * \param   length  Number of bytes to process.
 * \returns CRC-32 result. Returns 0xFFFFFFFF for zero-length input.
 * \note    Data is processed in 32-bit words (big-endian), matching the
 *          STM32 hardware CRC unit. Non-aligned trailing bytes are
 *          zero-padded to form a complete word.
 */
uint32_t SoftwareCrc32::Compute(const uint8_t* data, size_t length) const
{
    uint32_t crc = kInitValue;

    size_t fullWords = length / 4;
    for (size_t w = 0; w < fullWords; ++w)
    {
        uint32_t word = (static_cast<uint32_t>(data[0]) << 24) |
                        (static_cast<uint32_t>(data[1]) << 16) |
                        (static_cast<uint32_t>(data[2]) <<  8) |
                        (static_cast<uint32_t>(data[3]));
        data += 4;

        crc ^= word;
        for (int byte = 0; byte < 4; ++byte)
        {
            crc = (crc << 8) ^ mTable[(crc >> 24) & 0xFF];
        }
    }

    // Handle remaining bytes — zero-pad to form a complete word
    size_t remaining = length % 4;
    if (remaining > 0)
    {
        uint32_t word = 0;
        for (size_t i = 0; i < remaining; ++i)
        {
            word |= static_cast<uint32_t>(data[i]) << (24 - 8 * i);
        }

        crc ^= word;
        for (int byte = 0; byte < 4; ++byte)
        {
            crc = (crc << 8) ^ mTable[(crc >> 24) & 0xFF];
        }
    }

    return crc;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Builds the 256-entry CRC lookup table for polynomial 0x04C11DB7.
 */
void SoftwareCrc32::BuildTable()
{
    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t crc = i << 24;
        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x80000000u)
                crc = (crc << 1) ^ kPolynomial;
            else
                crc <<= 1;
        }
        mTable[i] = crc;
    }
}
