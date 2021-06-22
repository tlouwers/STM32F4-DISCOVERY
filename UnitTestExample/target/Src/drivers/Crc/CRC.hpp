/**
 * \file    CRC.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Crc
 *
 * \brief   Crc peripheral driver class.
 *
 * \details STM32F4 uses the CRC-32 (Ethernet) polynomial '0x4C11DB7'.
 *          X32 + X26 + X23 + X22 + X16 + X12 + X11 + X10 +X8 + X7 + X5 + X4 + X2+ X + 1
 *          The calculation is for 32 bit only and done via the hardware peripheral.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/CRC
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef CRC_HPP_
#define CRC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "Interfaces/IInitable.hpp"
#include "Interfaces/ICRC.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Crc final : public ICRC, public IInitable
{
public:
    Crc();
    virtual ~Crc();

    bool Init() override;
    bool IsInit() const override;
    bool Sleep() override;

    uint32_t Calculate(uint32_t* buffer, uint32_t length) override;

private:
    CRC_HandleTypeDef mHandle = {};
    bool              mInitialized;

    void CheckAndEnableAHBPeripheralClock();
    void CheckAndDisableAHBPeripheralClock();
};


#endif  // CRC_HPP_
