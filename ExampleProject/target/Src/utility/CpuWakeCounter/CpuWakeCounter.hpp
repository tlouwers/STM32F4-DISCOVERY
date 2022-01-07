/**
 * \file    CpuWakeCounter.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   CpuWakeCounter
 *
 * \brief   Helper class to measure CPU wake percentage.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/utility/CpuWakeCounter
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    02-2019
 */

#ifndef CPU_WAKE_COUNTER_HPP_
#define CPU_WAKE_COUNTER_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  CpuStats
 * \brief   Data structure containing statistics about the CPU.
 * \note    Assumed this is updated every second.
 */
struct CpuStats
{
    float    wakePercentage = 0.0f; ///< Percentage the CPU was awake (per second)
    uint32_t loopCount = 0;         ///< Number of times the main loop iterated (per second)
};


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    SleepMode
 * \brief   Available sleep modes.
 * \details The processor suspends execution (Clock is stopped) until one of
 *          the following events take place:
 *          - An IRQ interrupt, unless masked by the CPSR I Bit.
 *          - An FIQ interrupt, unless masked by the CPSR F Bit.
 *          - A Debug Entry request made to the processor and Debug is enabled.
 *          - An event is signaled by another processor using Send Event (only for WaitForEvent).
 *          - Another MP11 CPU return from exception (only for WaitForEvent).
 */
enum class SleepMode : bool
{
    WaitForInterrupt,
    WaitForEvent
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class CpuWakeCounter
{
public:
    bool Init();

    void EnterSleepMode(SleepMode mode, bool suspend_systick = true);

    bool IsUpdated() const;
    CpuStats GetStatistics() const;

private:
    CpuStats mCpuStats = {};
    bool mUpdateAvailable = false;
};


#endif  // CPU_WAKE_COUNTER_HPP_
