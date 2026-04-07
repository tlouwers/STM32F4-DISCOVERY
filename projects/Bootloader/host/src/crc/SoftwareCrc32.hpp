/**
 * \file    SoftwareCrc32.hpp
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
 * \details Polynomial: 0x04C11DB7 (CRC-32/MPEG-2), initial value 0xFFFFFFFF,
 *          no input/output reflection, no final XOR. Data is processed in
 *          32-bit words (big-endian), matching the STM32 hardware CRC unit.
 *          Non-aligned trailing bytes are zero-padded to form a complete word.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/crc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

#ifndef SOFTWARE_CRC32_HPP_
#define SOFTWARE_CRC32_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "ICrc.hpp"
#include <array>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class SoftwareCrc32 final : public ICrc
{
public:
    SoftwareCrc32();
    virtual ~SoftwareCrc32() = default;

    uint32_t Compute(const uint8_t* data, size_t length) const override;

private:
    std::array<uint32_t, 256> mTable;

    void BuildTable();
};


#endif  // SOFTWARE_CRC32_HPP_
