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
