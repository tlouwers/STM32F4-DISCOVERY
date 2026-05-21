/**
 * \file    Crc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Crc
 *
 * \brief   Fake CRC implementation that maps a small set of fixed inputs to
 *          deterministic reference outputs, so unit tests can assert exact
 *          CRC values without re-running the real polynomial.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake/drivers/Crc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "Crc.hpp"


bool Crc::Calculate(const uint32_t* buffer, uint32_t length, uint32_t& out)
{
    constexpr uint32_t reference[6] = { 0x01234567, 0x12345678, 0x23456789, 0x34567890, 0x45678901, 0x56789012 };

    if (buffer == nullptr) { return false; }
    if (length == 0)       { return false; }

    if (length == 6)
    {
        bool match = true;
        for (uint32_t i = 0; i < length; i++)
        {
            if (buffer[i] != reference[i])
            {
                match = false;
            }
        }

        if (match)
        {
            out = 0x63EC482A;
            return true;
        }
    }
    return false;
}
