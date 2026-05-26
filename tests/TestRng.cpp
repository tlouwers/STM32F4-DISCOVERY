/**
 * \file    TestRng.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the Rng driver, focused on the proactive
 *          reentrancy guard introduced in L12.
 *
 * \details The test exercises the real driver against a fake HAL surface
 *          (tests/Fake/stm32f4xx_hal_rng.cpp) whose
 *          HAL_RNG_GenerateRandomNumber tracks the maximum observed
 *          concurrent callers. The production atomic_flag guard must
 *          keep that count <= 1 regardless of how aggressively callers
 *          contend.
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
#include <thread>
#include <vector>

#include "gtest/gtest.h"

// Test subject
#include "drivers/Rng/Rng.hpp"

// Fake HAL observation hooks (declared inside extern "C" in the umbrella header)
extern "C" {
#include "stm32f4xx_hal.h"
}


namespace {


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class Rng_Test : public ::testing::Test
{
protected:
    Rng_Test()
    {
        FakeRNG_ResetObservation();
    }

    Rng mSubject;
};


/************************************************************************/
/* Tests                                                                */
/************************************************************************/
TEST_F(Rng_Test, GetRandom_NotInitialised_ReturnsFalse)
{
    uint32_t value = 0xDEADBEEFu;
    EXPECT_FALSE(mSubject.GetRandom(value));
    EXPECT_EQ(0xDEADBEEFu, value);    // out left untouched on failure
}

TEST_F(Rng_Test, Init_PllConfigured_Succeeds)
{
    EXPECT_TRUE(mSubject.Init());
    EXPECT_TRUE(mSubject.IsInit());
    EXPECT_TRUE(mSubject.Sleep());
    EXPECT_FALSE(mSubject.IsInit());
}

TEST_F(Rng_Test, GetRandom_Initialised_ReturnsValue)
{
    ASSERT_TRUE(mSubject.Init());

    uint32_t a = 0;
    uint32_t b = 0;
    EXPECT_TRUE(mSubject.GetRandom(a));
    EXPECT_TRUE(mSubject.GetRandom(b));
    EXPECT_NE(a, b);    // Fake increments a counter; consecutive values differ.
}

TEST_F(Rng_Test, GetRandom_ConcurrentCallers_NeverOverlap)
{
    ASSERT_TRUE(mSubject.Init());

    constexpr int kRoundsPerThread = 200;
    constexpr int kThreadCount     = 4;

    std::atomic<int> totalSuccess { 0 };
    std::atomic<int> totalContend { 0 };

    auto worker = [&]()
    {
        uint32_t local = 0;
        for (int i = 0; i < kRoundsPerThread; ++i)
        {
            if (mSubject.GetRandom(local))
            {
                totalSuccess.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                totalContend.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (int t = 0; t < kThreadCount; ++t)
    {
        workers.emplace_back(worker);
    }
    for (auto& th : workers) { th.join(); }

    // Direct proof of mutual exclusion: the fake observed how many callers
    // were simultaneously inside HAL_RNG_GenerateRandomNumber. The production
    // atomic_flag guard must keep this <= 1.
    EXPECT_EQ(1, FakeRNG_MaxObservedConcurrentCallers());

    // Every round either succeeded or hit the guard's contention path,
    // never both, never neither. Joins completing means no deadlock.
    EXPECT_EQ(kRoundsPerThread * kThreadCount,
              totalSuccess.load() + totalContend.load());

    // With a ~100 us critical section and 4 threads x 200 rounds, contention
    // is virtually certain. Zero false returns would mean the guard isn't
    // actually being tested -- treat that as a test-design failure.
    EXPECT_GT(totalContend.load(), 0);
}


} // namespace
