/**
 * \file    Mock_SPI.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   Mock_SPI
 *
 * \brief   GMock implementation of the ISPI interface.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Mock
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef MOCK_SPI_HPP_
#define MOCK_SPI_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "interfaces/ISPI.hpp"
#include "gmock/gmock.h"
#include <cstdint>
#include <cstring>


using ::testing::Return;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::_;


class Mock_SPI final : public ISPI
{
public:
    Mock_SPI()
    {
        ON_CALL(*this, WriteDMA(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, WriteReadDMA(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadDMA(_, _, _))
            .WillByDefault(Return(true));

        ON_CALL(*this, WriteInterrupt(_, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, WriteReadInterrupt(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, ReadInterrupt(_, _, _))
            .WillByDefault(Return(true));

        // A register read is always preceded by a one-byte address write, so
        // capture that address here; ReadBlocking then answers per-register
        // automatically, with no per-test wiring for the healthy-sensor path.
        ON_CALL(*this, WriteBlocking(_, _))
            .WillByDefault(DoAll(Invoke(this, &Mock_SPI::CaptureRegister), Return(true)));
        ON_CALL(*this, WriteReadBlocking(_, _, _))
            .WillByDefault(Return(true));
    }

    MOCK_METHOD3(WriteDMA, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(WriteReadDMA, bool(const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD3(ReadDMA, bool(uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD3(WriteInterrupt, bool(const uint8_t* src, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD4(WriteReadInterrupt, bool (const uint8_t* src, uint8_t* dest, uint16_t length, const std::function<void()>& handler));
    MOCK_METHOD3(ReadInterrupt, bool(uint8_t* dest, uint16_t length, const std::function<void()>& handler));

    MOCK_METHOD2(WriteBlocking, bool(const uint8_t* src, uint16_t length));
    MOCK_METHOD3(WriteReadBlocking, bool(const uint8_t* src, uint8_t* dest, uint16_t length));

    /**
     * \brief   Register-aware blocking read. Hand-written (not a GMock method)
     *          because SPI sensor reads are driven by the register address the
     *          caller just wrote. Single-byte reads answer from the per-register
     *          state below; multi-byte burst reads (e.g. a FIFO drain) succeed
     *          with zeros. Defaults model a healthy, empty sensor so existing
     *          tests need no setup.
     */
    bool ReadBlocking(uint8_t* dest, uint16_t length)
    {
        if (dest == nullptr) { return false; }
        if (length == 0)     { return false; }

        if (length == 1)
        {
            switch (mLastRegister)
            {
                case REG_WHO_AM_I:  dest[0] = mWhoAmI;   break;
                case REG_FIFO_SRC:  dest[0] = mFifoSrc;  break;
                case REG_FIFO_CTRL: dest[0] = mFifoCtrl; break;
                default:            dest[0] = 0x00;      break;
            }
        }
        else
        {
            std::memset(dest, 0, length);
        }

        return true;
    }

    /** \brief Override the WHO_AM_I answer (default = the LIS3DSH identifier). */
    void SetWhoAmI(uint8_t value)   { mWhoAmI   = value; }
    /** \brief Override the FIFO_SRC answer (default = empty). Set a non-empty
     *         value to drive the FIFO-drain path. */
    void SetFifoSrc(uint8_t value)  { mFifoSrc  = value; }
    /** \brief Override the FIFO_CTRL answer (default = 0). */
    void SetFifoCtrl(uint8_t value) { mFifoCtrl = value; }

private:
    static constexpr uint8_t REG_WHO_AM_I  = 0x0F;
    static constexpr uint8_t REG_FIFO_CTRL = 0x2E;
    static constexpr uint8_t REG_FIFO_SRC  = 0x2F;
    static constexpr uint8_t IDENTIFIER    = 0x3F;
    static constexpr uint8_t FIFO_EMPTY    = 0x20;

    /** \brief Capture the register address (RW / auto-increment bits masked off). */
    void CaptureRegister(const uint8_t* src, uint16_t length)
    {
        if ((src != nullptr) && (length >= 1)) { mLastRegister = src[0] & 0x3F; }
    }

    uint8_t mLastRegister = 0x00;
    uint8_t mWhoAmI       = IDENTIFIER;     ///< WHO_AM_I answer (healthy by default).
    uint8_t mFifoSrc      = FIFO_EMPTY;     ///< FIFO_SRC answer (empty by default).
    uint8_t mFifoCtrl     = 0x00;           ///< FIFO_CTRL answer.
};


#endif  // MOCK_SPI_HPP_
