/**
 * \file    Crc.hpp
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
 * \brief   Fake CRC driver for UnitTestExample native unit tests. Implements
 *          ICrc so test consumers can link without the STM32 HAL CRC unit.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/UnitTestExample/tests/Fake/drivers/Crc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef FAKE_CRC_HPP_
#define FAKE_CRC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "interfaces/ICrc.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Crc final : public ICrc
{
public:
    Crc() {}
    virtual ~Crc() {}

    bool Calculate(const uint32_t* buffer, uint32_t length, uint32_t& out) override;
};


#endif  // FAKE_CRC_HPP_
