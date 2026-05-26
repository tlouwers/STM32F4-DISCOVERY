/**
 * \file    IHI_M1388AR.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for the HI-M1388AR 8x8 LED matrix component.
 *
 * \details Extends IConfigInitable with the display-specific operations so
 *          application logic can depend on this contract and be
 *          unit-tested with a Mock_HI_M1388AR instead of the real
 *          SPI-backed driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef IHI_M1388AR_HPP_
#define IHI_M1388AR_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "interfaces/IInitable.hpp"


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
class IHI_M1388AR : public IConfigInitable
{
public:
    virtual ~IHI_M1388AR() = default;

    virtual bool ClearDisplay() = 0;
    virtual bool WriteDigits(const uint8_t* src) = 0;
};


#endif  // IHI_M1388AR_HPP_
