/**
 * \file    stm32f4xx_hal_rng.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake for the STM32F4 HAL RNG/RCC surface, written in C++
 *          so the contention observer can use std::atomic. C linkage is
 *          preserved on every entry point the driver under test calls into.
 *
 * \details Backs the RCC->CR / RCC->PLLCFGR fields with storage that already
 *          satisfies Rng::IsRngClockConfigured() (PLLRDY set, PLLQ in range),
 *          so Init() succeeds out of the box. HAL_RNG_GenerateRandomNumber
 *          deliberately sleeps ~100 microseconds while inside its critical
 *          section -- the production atomic_flag guard introduced in L12
 *          must keep concurrent callers from ever overlapping that window.
 *          A test inspects FakeRNG_MaxObservedConcurrentCallers() to verify.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include <chrono>
#include <cstdint>

extern "C" {
#include "stm32f4xx_hal.h"
}


/************************************************************************/
/* Helpers                                                              */
/************************************************************************/
namespace {

/**
 * \brief   Spin on real CPU work for the given microsecond budget.
 * \details Replaces std::this_thread::sleep_for, which on Windows / MinGW
 *          rounds sub-millisecond requests up (15+ ms granularity) or
 *          treats them as a yield -- neither widens the contention window
 *          enough to expose a missing guard. A steady_clock-bounded busy
 *          loop with a volatile sink gives deterministic, OS-independent
 *          time inside the critical section.
 */
inline void BusySpinFor(std::chrono::microseconds budget)
{
    const auto deadline = std::chrono::steady_clock::now() + budget;
    volatile uint32_t sink = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        sink = sink + 1U;
    }
    (void)sink;
}

} // namespace


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

RCC_TypeDef s_rcc = {
    RCC_CR_PLLRDY,                                  // CR: main PLL ready
    static_cast<uint32_t>(7U << RCC_PLLCFGR_PLLQ_Pos) // PLLCFGR: PLLQ = 7 (valid)
};
RNG_TypeDef s_rng = { 0U };

std::atomic<int>      g_concurrent_callers      { 0 };
std::atomic<int>      g_max_observed_concurrent { 0 };
std::atomic<uint32_t> g_counter                 { 0 };

} // namespace


/************************************************************************/
/* RNG / RCC fake instances (C linkage to match the driver's view)      */
/************************************************************************/
extern "C" RCC_TypeDef* const RCC = &s_rcc;
extern "C" RNG_TypeDef* const RNG = &s_rng;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_RNG_Init(RNG_HandleTypeDef* /*hrng*/)
{
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef HAL_RNG_DeInit(RNG_HandleTypeDef* /*hrng*/)
{
    return HAL_OK;
}

extern "C" HAL_StatusTypeDef HAL_RNG_GenerateRandomNumber(RNG_HandleTypeDef* /*hrng*/,
                                                         uint32_t* random32bit)
{
    if (random32bit == nullptr) { return HAL_ERROR; }

    const int n = g_concurrent_callers.fetch_add(1, std::memory_order_acq_rel) + 1;

    int prev = g_max_observed_concurrent.load(std::memory_order_acquire);
    while (n > prev &&
           !g_max_observed_concurrent.compare_exchange_weak(prev, n,
                                                            std::memory_order_acq_rel))
    {
        // CAS retry; prev was reloaded by compare_exchange_weak
    }

    // Widen the critical section so any caller that races past the production
    // atomic_flag guard would necessarily overlap with the current caller.
    // Use a CPU busy-spin, not std::this_thread::sleep_for: on Windows/MinGW,
    // sub-millisecond sleeps either round up to ~15 ms or are treated as a
    // yield, neither of which keeps the thread inside the critical section
    // long enough to expose a missing guard.
    BusySpinFor(std::chrono::microseconds(50));

    *random32bit = g_counter.fetch_add(1, std::memory_order_relaxed);

    g_concurrent_callers.fetch_sub(1, std::memory_order_acq_rel);
    return HAL_OK;
}


/************************************************************************/
/* Test-only observation hooks                                          */
/************************************************************************/
extern "C" int FakeRNG_MaxObservedConcurrentCallers(void)
{
    return g_max_observed_concurrent.load(std::memory_order_acquire);
}

extern "C" void FakeRNG_ResetObservation(void)
{
    g_concurrent_callers.store(0,      std::memory_order_release);
    g_max_observed_concurrent.store(0, std::memory_order_release);
    g_counter.store(0,                 std::memory_order_release);
}
