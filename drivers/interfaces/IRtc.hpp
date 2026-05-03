/**
 * \file    IRtc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for Rtc peripheral driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IRTC_HPP_
#define IRTC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  DateTime
 * \brief   Structure to contain date and time information.
 */
struct DateTime
{
    uint16_t year   = 2000;     ///< The year [2000..2099]. The RTC year register is BCD 00..99.
    uint8_t  month  = 1;        ///< The month [1..12].
    uint8_t  day    = 1;        ///< The day [1..31].
    uint8_t  hour   = 0;        ///< The hour [0..23].
    uint8_t  minute = 0;        ///< The minutes [0..59].
    uint8_t  second = 0;        ///< The seconds [0..59].
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class IRtc
{
public:
    virtual bool SetDateTime(const DateTime& dateTime) = 0;
    virtual bool GetDateTime(DateTime& dateTime) = 0;
};


#endif  // IRTC_HPP_
