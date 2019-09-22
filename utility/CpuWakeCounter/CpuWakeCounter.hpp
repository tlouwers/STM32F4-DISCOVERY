/**
 * \file CpuWakeCounter.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Helper class intended to put the CPU into a 'light' sleep mode and
 *          measure the wake percentage in one go.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/utility/CpuWakeCounter
 *
 * \details This code is intended to be used as tracing mechanism. It measures
 *          the wake percentage of the CPU and keeps track of the (main) loop
 *          count.
 *          At the end of the main loop call 'EnterSleepMode()' to sleep with
 *          either '__WFI()' or '__WFE()'. An interrupt is needed to wake the
 *          CPU again (read the ARM documentation for all possible wakeup
 *          sources).
 *          By replacing the default enter/exit sleep mode call, some
 *          functionality is added to calculate the wake time of the CPU.
 *          Roughly each second an update of CpuStats is made available where
 *          details can be retrieved for the application to use.
 *          This code makes use of the DWT block as present on an ARM
 *          Cortex-M3, M4 or M7. This code was developed for an STM32F407G,
 *          but can be adapted for other microcontrollers as well.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        02-2019
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
/**
 * \brief   Helper class to measure CPU wake percentage.
 */
class CpuWakeCounter
{
public:
    void EnterSleepMode(SleepMode mode, bool suspend_systick = true);

    bool IsUpdated() const;
    CpuStats GetStatistics() const;

private:
    CpuStats mCpuStats = {};
    bool mUpdateAvailable = false;
};


#endif  // CPU_WAKE_COUNTER_HPP_
