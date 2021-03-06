/**
 * \file    ISPI.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for SPI (master) peripheral driver.
 *
 *          The Asynchronous methods are to be implemented using peripheral
 *          DMA or interrupts, meaning can be called and return immediately.
 *          The transaction is handled, once complete this is signaled
 *          by the callback.
 *
 * \note    The data pointers (src, dest) can be 'nullptr'.
 *          Sending a length of 0 is permitted, but is to return false.
 *          When not initialized, all write/read calls are to return false.
 *
 *          Bus arbitration is not specified, but assumed to be implemented
 *          in low level drivers.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

#ifndef ISPI_HPP_
#define ISPI_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class ISPI
{
public:
    virtual bool WriteDMA(const uint8_t* src, uint16_t length, const std::function<void()>& handler) = 0;
    virtual bool WriteReadDMA(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler) = 0;
    virtual bool ReadDMA(uint8_t* dest, uint16_t length, const std::function<void()>& handler) = 0;

    virtual bool WriteInterrupt(const uint8_t* src, uint16_t length, const std::function<void()>& handler) = 0;
    virtual bool WriteReadInterrupt(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler) = 0;
    virtual bool ReadInterrupt(uint8_t* dest, uint16_t length, const std::function<void()>& handler) = 0;

    virtual bool WriteBlocking(const uint8_t* src, uint16_t length) = 0;
    virtual bool WriteReadBlocking(const uint8_t* src, uint8_t* dest, uint16_t length) = 0;
    virtual bool ReadBlocking(uint8_t* dest, uint16_t length) = 0;
};


#endif  // ISPI_HPP_
