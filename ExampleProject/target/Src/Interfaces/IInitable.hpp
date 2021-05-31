/**
 * \file    IInitable.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Initable interface class.
 *
 * \details This class is intended to act as interface for peripheral drivers
 *          and components, making sure they have a similar interface for
 *          basic Init and Sleep functionality.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

#ifndef IINITABLE_HPP_
#define IINITABLE_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
/**
 * \brief   IConfig interface class.
 * \details Implemented to act as base class for derived specialized
 *          Config classes.
 */
class IConfig
{
public:
    IConfig() {};
};


/**
 * \brief   IInitable interface class.
 */
class IInitable
{
public:
    virtual bool Init() = 0;
    virtual bool IsInit() const = 0;
    virtual bool Sleep() = 0;
};


/**
 * \brief   IConfigInitable interface class, with config object.
 */
class IConfigInitable
{
public:
    virtual bool Init(const IConfig& config) = 0;
    virtual bool IsInit() const = 0;
    virtual bool Sleep() = 0;
};


#endif  // IINITABLE_HPP_
