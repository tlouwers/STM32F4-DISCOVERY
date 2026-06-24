/**
 * \file    IRng.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Generic interface for Rng peripheral driver.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/interfaces
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef IRNG_HPP_
#define IRNG_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class IRng
{
public:
    virtual ~IRng() = default;

    /**
     * \brief   Get a hardware-generated random number.
     * \param   out     Receives the generated random number on success;
     *                  left unchanged on failure.
     * \returns True if a random number was generated, else false (peripheral
     *          not initialised, contention with a concurrent caller, or a HAL
     *          clock/seed/timeout error).
     */
    virtual bool GetRandom(uint32_t& out) = 0;
};


#endif  // IRNG_HPP_
