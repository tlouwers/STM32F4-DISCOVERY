/**
 * \file    SerialWin.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   SerialWin
 *
 * \brief   Windows serial port implementation (stub).
 *
 * \details Will use Win32 CreateFile/ReadFile/WriteFile with DCB configuration.
 *          Currently a compile-only stub — real implementation to be added when
 *          testing on Windows.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

#ifndef SERIAL_WIN_HPP_
#define SERIAL_WIN_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "ISerial.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class SerialWin final : public ISerial
{
public:
    SerialWin()  = default;
    ~SerialWin() = default;

    bool Open(const std::string& port, const SerialConfig& config) override;
    void Close() override;
    bool IsOpen() const override;

    int  Write(const uint8_t* data, size_t length) override;
    int  Read(uint8_t* buffer, size_t length) override;

    void SetTimeout(uint32_t timeoutMs) override;
    void FlushInput() override;

private:
    bool mOpen = false;
};


#endif  // SERIAL_WIN_HPP_
