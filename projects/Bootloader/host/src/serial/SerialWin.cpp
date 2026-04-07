/**
 * \file    SerialWin.cpp
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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "serial/SerialWin.hpp"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Opens a serial port (stub — always returns false).
 * \returns False (not implemented).
 */
bool SerialWin::Open(const std::string& /*port*/, const SerialConfig& /*config*/)
{
    return false;
}

/**
 * \brief   Closes the serial port (stub).
 */
void SerialWin::Close()
{
    mOpen = false;
}

/**
 * \brief   Indicates if the serial port is open.
 * \returns False (stub).
 */
bool SerialWin::IsOpen() const
{
    return mOpen;
}

/**
 * \brief   Writes data (stub — always returns -1).
 */
int SerialWin::Write(const uint8_t* /*data*/, size_t /*length*/)
{
    return -1;
}

/**
 * \brief   Reads data (stub — always returns -1).
 */
int SerialWin::Read(uint8_t* /*buffer*/, size_t /*length*/)
{
    return -1;
}

/**
 * \brief   Sets the read timeout (stub — no-op).
 */
void SerialWin::SetTimeout(uint32_t /*timeoutMs*/)
{
}

/**
 * \brief   Flushes input (stub — no-op).
 */
void SerialWin::FlushInput()
{
}
