/**
 * \file    SerialPosix.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SerialPosix
 *
 * \brief   Linux/POSIX serial port implementation using termios.
 *
 * \details Configures the port for raw mode with the specified baud rate,
 *          parity, and stop bits. Uses select() for read timeout control.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

#ifndef SERIAL_POSIX_HPP_
#define SERIAL_POSIX_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "ISerial.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class SerialPosix final : public ISerial
{
public:
    SerialPosix();
    virtual ~SerialPosix();

    bool Open(const std::string& port, const SerialConfig& config) override;
    void Close() override;
    bool IsOpen() const override;

    int  Write(const uint8_t* data, size_t length) override;
    int  Read(uint8_t* buffer, size_t length) override;

    void SetTimeout(uint32_t timeoutMs) override;
    void FlushInput() override;

private:
    int      mFd;
    uint32_t mTimeoutMs;

    bool ApplyConfig(const SerialConfig& config);
};


#endif  // SERIAL_POSIX_HPP_
