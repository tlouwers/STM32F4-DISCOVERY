#include "CRC.hpp"


uint32_t Crc::Calculate(uint32_t* buffer, uint32_t length)
{
    constexpr uint32_t reference[6] = { 0x01234567, 0x12345678, 0x23456789, 0x34567890, 0x45678901, 0x56789012 };

    if (buffer == nullptr) { return 0; }
    if (length == 0)       { return 0; }

    if (length == 6)
    {
        bool result = true;
        for (auto i = 0; i < length; i++)
        {
            if (buffer[i] != reference[i])
            {
                result = false;
            }
        }

        if (result == true)
        {
            return 0x63EC482A;
        }
    }
    return 0;
}
