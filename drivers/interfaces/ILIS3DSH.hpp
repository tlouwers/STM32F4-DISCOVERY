/**
 * \file    ILIS3DSH.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for the LIS3DSH accelerometer component.
 *
 * \details Extends IConfigInitable with the accelerometer-specific
 *          operations so application logic can depend on this contract
 *          and be unit-tested with a Mock_LIS3DSH instead of the real
 *          SPI-backed driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef ILIS3DSH_HPP_
#define ILIS3DSH_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
class ILIS3DSH : public IConfigInitable
{
public:
    virtual ~ILIS3DSH() = default;

    virtual bool Enable() = 0;
    virtual bool Disable() = 0;
    virtual void SetHandler(const std::function<void(uint8_t length)>& handler) = 0;
    virtual bool RetrieveAxesData(uint8_t* dest, uint8_t length) = 0;
};


#endif  // ILIS3DSH_HPP_
