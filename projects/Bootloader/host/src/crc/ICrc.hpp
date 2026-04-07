/**
 * \file    ICrc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for software CRC calculation.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/crc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

#ifndef ICRC_HPP_
#define ICRC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstddef>
#include <cstdint>


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
class ICrc
{
public:
    virtual ~ICrc() = default;

    virtual uint32_t Compute(const uint8_t* data, size_t length) const = 0;
};


#endif  // ICRC_HPP_
