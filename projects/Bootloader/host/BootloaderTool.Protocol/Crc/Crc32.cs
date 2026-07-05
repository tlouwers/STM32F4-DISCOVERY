// ----------------------------------------------------------------------------
//  Crc32.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Software CRC-32 implementation matching the STM32F4 hardware CRC peripheral.
//  Polynomial 0x04C11DB7, init 0xFFFFFFFF, no reflection, no final XOR
//  (CRC-32/MPEG-2). Data is processed in big-endian 32-bit words to match the
//  hardware unit; trailing bytes are zero-padded.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    04-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Crc;

/// <summary>
/// Software CRC-32/MPEG-2 matching the STM32F4 hardware CRC peripheral.
/// </summary>
public sealed class Crc32 : ICrc
{
    private const uint Polynomial = 0x04C11DB7u;
    private const uint InitValue  = 0xFFFFFFFFu;

    private readonly uint[] _table = new uint[256];

    public Crc32()
    {
        BuildTable();
    }

    /// <summary>
    /// Computes the CRC-32/MPEG-2 over the given byte buffer.
    /// Data is processed in 32-bit big-endian words; trailing bytes are zero-padded.
    /// </summary>
    /// <param name="data">Input bytes.</param>
    /// <returns>CRC-32 result. Returns 0xFFFFFFFF for empty input.</returns>
    public uint Compute(ReadOnlySpan<byte> data)
    {
        uint crc = InitValue;

        int fullWords = data.Length / 4;
        int offset = 0;

        for (int w = 0; w < fullWords; w++)
        {
            uint word = ((uint)data[offset]     << 24) |
                        ((uint)data[offset + 1] << 16) |
                        ((uint)data[offset + 2] <<  8) |
                        ((uint)data[offset + 3]);
            offset += 4;

            crc ^= word;
            for (int b = 0; b < 4; b++)
            {
                crc = (crc << 8) ^ _table[(crc >> 24) & 0xFF];
            }
        }

        // Handle remaining bytes -- zero-pad to form a complete word
        int remaining = data.Length % 4;
        if (remaining > 0)
        {
            uint word = 0;
            for (int i = 0; i < remaining; i++)
            {
                word |= (uint)data[offset + i] << (24 - 8 * i);
            }

            crc ^= word;
            for (int b = 0; b < 4; b++)
            {
                crc = (crc << 8) ^ _table[(crc >> 24) & 0xFF];
            }
        }

        return crc;
    }

    private void BuildTable()
    {
        for (uint i = 0; i < 256; i++)
        {
            uint crc = i << 24;
            for (int bit = 0; bit < 8; bit++)
            {
                if ((crc & 0x80000000u) != 0)
                    crc = (crc << 1) ^ Polynomial;
                else
                    crc <<= 1;
            }
            _table[i] = crc;
        }
    }
}
