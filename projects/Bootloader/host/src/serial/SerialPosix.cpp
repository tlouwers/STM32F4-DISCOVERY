/**
 * \file    SerialPosix.cpp
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
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/host/src/serial
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    04-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "serial/SerialPosix.hpp"

#ifdef __linux__

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares internal state.
 */
SerialPosix::SerialPosix() :
    mFd(-1),
    mTimeoutMs(2000)
{
}

/**
 * \brief   Destructor, closes the port if open.
 */
SerialPosix::~SerialPosix()
{
    Close();
}

/**
 * \brief   Opens a serial port with the given configuration.
 * \param   port    Device path (e.g. "/dev/ttyUSB0").
 * \param   config  Baud rate, parity, stop bits, and timeout settings.
 * \returns True if the port was opened and configured successfully.
 */
bool SerialPosix::Open(const std::string& port, const SerialConfig& config)
{
    if (mFd >= 0)
        Close();

    mFd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (mFd < 0)
        return false;

    // Clear non-blocking after open (we use select for timeout)
    int flags = fcntl(mFd, F_GETFL, 0);
    if (flags < 0 || fcntl(mFd, F_SETFL, flags & ~O_NONBLOCK) < 0)
    {
        ::close(mFd);
        mFd = -1;
        return false;
    }

    mTimeoutMs = config.timeoutMs;

    if (!ApplyConfig(config))
    {
        ::close(mFd);
        mFd = -1;
        return false;
    }

    return true;
}

/**
 * \brief   Closes the serial port.
 */
void SerialPosix::Close()
{
    if (mFd >= 0)
    {
        ::close(mFd);
        mFd = -1;
    }
}

/**
 * \brief   Indicates if the serial port is currently open.
 * \returns True if the port is open.
 */
bool SerialPosix::IsOpen() const
{
    return mFd >= 0;
}

/**
 * \brief   Writes data to the serial port.
 * \param   data    Pointer to the data buffer to send.
 * \param   length  Number of bytes to send.
 * \returns Number of bytes written, or -1 on error.
 */
int SerialPosix::Write(const uint8_t* data, size_t length)
{
    if (mFd < 0 || data == nullptr || length == 0)
        return -1;

    size_t totalWritten = 0;
    while (totalWritten < length)
    {
        ssize_t n = ::write(mFd, data + totalWritten, length - totalWritten);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        totalWritten += static_cast<size_t>(n);
    }

    tcdrain(mFd);
    return static_cast<int>(totalWritten);
}

/**
 * \brief   Reads data from the serial port, honouring the configured timeout.
 * \param   buffer  Destination buffer.
 * \param   length  Maximum number of bytes to read.
 * \returns Number of bytes read, 0 on timeout, or -1 on error.
 */
int SerialPosix::Read(uint8_t* buffer, size_t length)
{
    if (mFd < 0 || buffer == nullptr || length == 0)
        return -1;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(mFd, &readSet);

    struct timeval tv;
    tv.tv_sec  = mTimeoutMs / 1000;
    tv.tv_usec = (mTimeoutMs % 1000) * 1000;

    int sel = select(mFd + 1, &readSet, nullptr, nullptr, &tv);
    if (sel < 0)
        return -1;       // error
    if (sel == 0)
        return 0;        // timeout

    ssize_t n = ::read(mFd, buffer, length);
    if (n < 0)
        return -1;

    return static_cast<int>(n);
}

/**
 * \brief   Sets the read timeout.
 * \param   timeoutMs   Timeout in milliseconds.
 */
void SerialPosix::SetTimeout(uint32_t timeoutMs)
{
    mTimeoutMs = timeoutMs;
}

/**
 * \brief   Flushes all pending input data.
 */
void SerialPosix::FlushInput()
{
    if (mFd >= 0)
        tcflush(mFd, TCIFLUSH);
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Applies baud rate, parity, stop bits, and raw mode to the open fd.
 * \param   config  Serial configuration to apply.
 * \returns True if all termios settings were applied successfully.
 */
bool SerialPosix::ApplyConfig(const SerialConfig& config)
{
    struct termios tty;
    std::memset(&tty, 0, sizeof(tty));

    if (tcgetattr(mFd, &tty) != 0)
        return false;

    // Baud rate
    speed_t speed;
    switch (config.baudRate)
    {
        case 1200:   speed = B1200;   break;
        case 2400:   speed = B2400;   break;
        case 4800:   speed = B4800;   break;
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        default:     return false;
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    // Data bits
    tty.c_cflag &= ~CSIZE;
    switch (config.dataBits)
    {
        case 7:  tty.c_cflag |= CS7; break;
        case 8:  tty.c_cflag |= CS8; break;
        default: return false;
    }

    // Parity
    switch (config.parity)
    {
        case Parity::None:
            tty.c_cflag &= ~PARENB;
            break;
        case Parity::Even:
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            tty.c_iflag |= INPCK;
            break;
        case Parity::Odd:
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            tty.c_iflag |= INPCK;
            break;
    }

    // Stop bits
    if (config.stopBits == StopBits::Two)
        tty.c_cflag |= CSTOPB;
    else
        tty.c_cflag &= ~CSTOPB;

    // Raw mode — no echo, no canonical, no signals, no flow control
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | ICRNL | INLCR | IGNCR);
    tty.c_oflag &= ~OPOST;

    // VMIN/VTIME: return immediately with whatever is available
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(mFd, TCSANOW, &tty) != 0)
        return false;

    tcflush(mFd, TCIOFLUSH);
    return true;
}

#endif  // __linux__
