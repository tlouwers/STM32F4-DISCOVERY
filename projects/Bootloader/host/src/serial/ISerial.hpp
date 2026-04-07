/**
 * \file    ISerial.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for serial port communication.
 *
 * \details Implementations handle OS-specific serial I/O (termios on Linux,
 *          Win32 CreateFile on Windows). The ST AN3155 bootloader protocol
 *          requires 8E1 (8 data bits, even parity, 1 stop bit).
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

#ifndef ISERIAL_HPP_
#define ISERIAL_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstddef>
#include <cstdint>
#include <string>


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
enum class Parity : uint8_t
{
    None,
    Even,
    Odd
};

enum class StopBits : uint8_t
{
    One,
    Two
};


/************************************************************************/
/* Structs                                                              */
/************************************************************************/
struct SerialConfig
{
    uint32_t baudRate  = 115200;
    uint8_t  dataBits  = 8;
    Parity   parity    = Parity::Even;   // AN3155 requires even parity
    StopBits stopBits  = StopBits::One;
    uint32_t timeoutMs = 2000;
};


/************************************************************************/
/* Interface declaration                                                */
/************************************************************************/
class ISerial
{
public:
    virtual ~ISerial() = default;

    virtual bool Open(const std::string& port, const SerialConfig& config) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;

    virtual int  Write(const uint8_t* data, size_t length) = 0;
    virtual int  Read(uint8_t* buffer, size_t length) = 0;

    virtual void SetTimeout(uint32_t timeoutMs) = 0;
    virtual void FlushInput() = 0;
};


#endif  // ISERIAL_HPP_
