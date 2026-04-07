/**
 * \file    MockSerial.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Google Mock implementation of ISerial for unit testing.
 *
 * \details Scriptable mock serial port. Queue expected writes and canned
 *          responses (ACK, NACK, timeout, partial data) to simulate the
 *          ST bootloader without hardware.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

#ifndef MOCK_SERIAL_HPP_
#define MOCK_SERIAL_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "serial/ISerial.hpp"
#include <gmock/gmock.h>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class MockSerial : public ISerial
{
public:
    MOCK_METHOD(bool, Open, (const std::string& port, const SerialConfig& config), (override));
    MOCK_METHOD(void, Close, (), (override));
    MOCK_METHOD(bool, IsOpen, (), (const, override));
    MOCK_METHOD(int,  Write, (const uint8_t* data, size_t length), (override));
    MOCK_METHOD(int,  Read,  (uint8_t* buffer, size_t length), (override));
    MOCK_METHOD(void, SetTimeout, (uint32_t timeoutMs), (override));
    MOCK_METHOD(void, FlushInput, (), (override));
};


#endif  // MOCK_SERIAL_HPP_
