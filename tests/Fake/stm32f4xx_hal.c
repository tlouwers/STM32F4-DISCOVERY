/**
 * \file    stm32f4xx_hal.c
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Fake STM32F4 HAL implementation for the native unit-test build.
 *          Provides no-op or trivial bodies for the HAL entry points the
 *          drivers reach into. Kept in 'C' to avoid multiple-definition
 *          conflicts with the umbrella header.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#include "stm32f4xx_hal.h"

// Fake implementation done in 'C' file, to prevent multiple definitions.


// Cortex-M4 DWT / CoreDebug register storage (cycle-counter busy-waits read these).
static DWT_TypeDef       s_dwt        = { 0 };
static CoreDebug_TypeDef s_core_debug = { 0 };
DWT_TypeDef*       const DWT       = &s_dwt;
CoreDebug_TypeDef* const CoreDebug = &s_core_debug;

// Test control for the fake cycle counter (CpuWakeCounter). When counting is
// enabled, executing instructions advances CYCCNT -- modelled by __NOP() and the
// WFI/WFE intrinsics below -- so Init()'s "is the counter running?" probe passes
// and a sleep can be made to consume cycles. Defaults reproduce a healthy,
// idle counter; the I2C busy-wait is unaffected because SystemCoreClock stays 0.
static int      s_dwt_counting      = 1;    ///< 0 freezes CYCCNT (drives Init's fail path).
static uint32_t s_dwt_sleep_advance = 0U;   ///< Cycles each __WFI/__WFE adds to CYCCNT.

// __NOP() formally part of CMSIS, only available for ARM. Modelled to advance the
// cycle counter so CpuWakeCounter::Init() observes a running DWT.
void __NOP(void) { if (s_dwt_counting) { s_dwt.CYCCNT++; } }

// Cortex-M core intrinsics used by CpuWakeCounter -- no-ops / trivial on the host.
uint32_t __get_PRIMASK(void)            { return 0U; }
void     __disable_irq(void)            { ; }
void     __set_PRIMASK(uint32_t mask)   { (void)mask; }
void     __WFI(void)                    { s_dwt.CYCCNT += s_dwt_sleep_advance; }
void     __WFE(void)                    { s_dwt.CYCCNT += s_dwt_sleep_advance; }

// Systick suspend/resume around sleep -- no-ops on the native build.
void HAL_SuspendTick(void) { ; }
void HAL_ResumeTick(void)  { ; }

// Test hooks for the fake cycle counter.
void FakeDWT_Reset(void)
{
    s_dwt.CYCCNT        = 0U;
    s_dwt.CTRL          = 0U;
    s_core_debug.DEMCR  = 0U;
    s_dwt_counting      = 1;
    s_dwt_sleep_advance = 0U;
}
void FakeDWT_SetCounting(int enabled)          { s_dwt_counting = enabled; }
void FakeDWT_SetSleepAdvance(uint32_t cycles)  { s_dwt_sleep_advance = cycles; }

void HAL_Delay(uint32_t Delay) { (void)Delay; }


// NVIC controls: no-ops on the native build (no interrupt controller).
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority)
{
    (void)IRQn; (void)PreemptPriority; (void)SubPriority;
}
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn)       { (void)IRQn; }
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn)      { (void)IRQn; }
void HAL_NVIC_ClearPendingIRQ(IRQn_Type IRQn) { (void)IRQn; }


// APB bus clocks: the drafted 168 MHz PLL gives APB1 = 42 MHz, APB2 = 84 MHz.
// Fixed values are enough for the drivers' bus-speed validation / prescaler math.
uint32_t HAL_RCC_GetPCLK1Freq(void) { return 42000000U; }
uint32_t HAL_RCC_GetPCLK2Freq(void) { return 84000000U; }
uint32_t HAL_RCC_GetHCLKFreq(void)  { return 168000000U; }


// Zero so every DWT cycle-counter busy-wait is a no-op (see stm32f4xx_hal.h).
uint32_t SystemCoreClock = 0U;
