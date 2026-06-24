/**
 * \file    Rng.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Rng
 *
 * \brief   Hardware random number generator class.
 *
 * \details According to specification: Rng passed FIPS PUB 140-2 (2001 October 10)
 *          tests with a success ratio of 99%.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/Rng
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef RNG_HPP_
#define RNG_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include <cstdint>
#include "interfaces/IInitable.hpp"
#include "interfaces/IRng.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Rng final : public IRng, public IInitable
{
public:
    Rng();
    virtual ~Rng();

    bool Init() override;
    bool IsInit() const override;
    bool Sleep() override;

    bool GetRandom(uint32_t& out) override;

    // Explicit disabled constructors/operators
    Rng(const Rng&)            = delete;
    Rng& operator=(const Rng&) = delete;
    Rng(Rng&&)                 = delete;
    Rng& operator=(Rng&&)      = delete;

private:
    RNG_HandleTypeDef  mHandle = {};
    std::atomic<bool>  mInitialized{false};        ///< Atomic: read by GetRandom() while Init()/Sleep() may run in another context
    std::atomic_flag   mInUse = ATOMIC_FLAG_INIT;  ///< Reentrancy guard around HAL_RNG_GenerateRandomNumber

    void CheckAndEnablePeripheralClock();
    void CheckAndDisablePeripheralClock();
    static bool IsRngClockConfigured();
};


#endif  // RNG_HPP_
