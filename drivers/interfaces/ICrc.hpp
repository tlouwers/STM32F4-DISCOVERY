/**
 * \file    ICrc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for Crc peripheral.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef ICRC_HPP_
#define ICRC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class ICrc
{
public:
    virtual ~ICrc() = default;
    /**
     * \brief   Calculate a CRC over the given buffer.
     * \param   buffer  Pointer to the first 32-bit word in the buffer.
     * \param   length  Number of 32-bit words in the buffer.
     * \param   out     Output parameter; set to the computed CRC on success,
     *                  left untouched on failure.
     * \returns True if the CRC was computed and written to \p out, false on
     *          bad parameters (null buffer, zero length), an uninitialised
     *          peripheral, or a re-entrant call (see note).
     * \note    The underlying CRC unit is a single shared accumulator and is
     *          NOT re-entrant: a concurrent call from another task or an ISR is
     *          rejected (returns false) rather than allowed to corrupt the
     *          in-flight computation. Call from a single context, or serialise.
     */
    virtual bool Calculate(const uint32_t* buffer, uint32_t length, uint32_t& out) = 0;
};


#endif  // ICRC_HPP_
