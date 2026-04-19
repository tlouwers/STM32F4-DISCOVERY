/**
 * \file IRtc.hpp
 *
 * \brief   Rtc interface class.
 *
 * \details This class is intended to act as interface for the Rtc class, to
 *          ease unit testing.
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
    uint16_t year   = 2000;     ///< The year [2000..2255].
    uint8_t  month  = 1;        ///< The month [1..12].
    uint8_t  day    = 1;        ///< The day [1..31].
    uint8_t  hour   = 0;        ///< The hour [0..23].
    uint8_t  minute = 0;        ///< The minutes [0..59].
    uint8_t  second = 0;        ///< The seconds [0..59].
};


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
/**
 * \brief   IRtc interface class.
 */
class IRtc
{
public:
    virtual bool SetDateTime(const DateTime& dateTime) = 0;
    virtual bool GetDateTime(DateTime& dateTime) = 0;
};


#endif  // IRTC_HPP_
