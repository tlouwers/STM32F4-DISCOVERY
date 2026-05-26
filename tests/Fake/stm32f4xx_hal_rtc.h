/**
 * \file    stm32f4xx_hal_rtc.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL RTC surface for the unit-test build.
 *          Declares the time/date structs, the hour-format / output / format
 *          and RCC RTC clock-select macros, the RCC RTC clock-gating macros,
 *          and the HAL_RTC_* prototypes the real Rtc driver compiles against.
 *          The RTC types/instance themselves live in the umbrella
 *          stm32f4xx_hal.h (Rtc.hpp pulls in only the umbrella for its handle
 *          member).
 *
 * \details Every HAL_RTC_* entry point returns HAL_OK by default; the
 *          FakeRTC_Set*Result hooks drive each failure branch independently
 *          (Init, DeInit, SetTime, GetTime, SetDate, GetDate). SetTime/SetDate
 *          store into fake registers and GetTime/GetDate read them back, so a
 *          driver-level set/get round-trip is observable. The backup-register
 *          surface (stm32f4xx_hal_rtc_ex.h) is backed by the same fake state so
 *          the cold/warm-boot magic detection in Rtc::Init can be exercised.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_RTC_H
#define __STM32F4xx_HAL_RTC_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \brief   RTC time -- field names mirror the real HAL so Rtc::SetDateTime /
 *          GetDateTime populate/read it exactly as on hardware.
 */
typedef struct
{
    uint8_t Hours;     ///< Hour   [0..23] in 24h format
    uint8_t Minutes;   ///< Minute [0..59]
    uint8_t Seconds;   ///< Second [0..59]
} RTC_TimeTypeDef;

/**
 * \brief   RTC date -- field names mirror the real HAL. Year is the BCD-style
 *          [0..99] offset from 2000 the driver applies.
 */
typedef struct
{
    uint8_t WeekDay;   ///< RTC_WEEKDAY_* (unused by the driver)
    uint8_t Month;     ///< Month [1..12]
    uint8_t Date;      ///< Day   [1..31]
    uint8_t Year;      ///< Year  [0..99] (offset from 2000)
} RTC_DateTypeDef;


/************************************************************************/
/* Config-value macros (mirror the real HAL)                            */
/************************************************************************/
#define RTC_HOURFORMAT_24            ((uint32_t)0x00000000U)

#define RTC_OUTPUT_DISABLE           ((uint32_t)0x00000000U)
#define RTC_OUTPUT_POLARITY_HIGH     ((uint32_t)0x00000000U)
#define RTC_OUTPUT_TYPE_OPENDRAIN    ((uint32_t)0x00000000U)

#define RTC_FORMAT_BIN               ((uint32_t)0x00000000U)
#define RTC_FORMAT_BCD               ((uint32_t)0x00000001U)


/************************************************************************/
/* RCC RTC clock select + gating (mirror the real HAL -- no-ops here)   */
/************************************************************************/
#define RCC_RTCCLKSOURCE_LSI         ((uint32_t)0x00000200U)
#define RCC_RTCCLKSOURCE_LSE         ((uint32_t)0x00000100U)
#define RCC_RTCCLKSOURCE_HSE_DIV8    ((uint32_t)0x00000300U)

/* Selecting the clock source / enabling-disabling the RTC clock has no native
   effect; consume the argument so `-Wunused` stays quiet. */
#define __HAL_RCC_RTC_CONFIG(__RTCCLKSource__)   do { (void)(__RTCCLKSource__); } while(0)
#define __HAL_RCC_RTC_ENABLE()                   do { } while(0)
#define __HAL_RCC_RTC_DISABLE()                  do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_rtc.cpp)          */
/************************************************************************/
HAL_StatusTypeDef HAL_RTC_Init(RTC_HandleTypeDef* hrtc);
HAL_StatusTypeDef HAL_RTC_DeInit(RTC_HandleTypeDef* hrtc);
HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef* hrtc, RTC_TimeTypeDef* sTime, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef* hrtc, RTC_TimeTypeDef* sTime, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_SetDate(RTC_HandleTypeDef* hrtc, RTC_DateTypeDef* sDate, uint32_t Format);
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef* hrtc, RTC_DateTypeDef* sDate, uint32_t Format);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake RTC state: every result to HAL_OK, time/date + backup regs to 0. */
void FakeRTC_Reset(void);

void FakeRTC_SetInitResult(HAL_StatusTypeDef result);     ///< Drives Init failure
void FakeRTC_SetDeInitResult(HAL_StatusTypeDef result);   ///< Drives Sleep failure
void FakeRTC_SetSetTimeResult(HAL_StatusTypeDef result);  ///< Drives SetDateTime SetTime failure
void FakeRTC_SetGetTimeResult(HAL_StatusTypeDef result);  ///< Drives GetDateTime GetTime failure
void FakeRTC_SetSetDateResult(HAL_StatusTypeDef result);  ///< Drives SetDateTime SetDate failure
void FakeRTC_SetGetDateResult(HAL_StatusTypeDef result);  ///< Drives GetDateTime GetDate failure

void     FakeRTC_SetBackupRegister(uint32_t reg, uint32_t value);  ///< Preset a backup reg (e.g. the warm-boot magic)
uint32_t FakeRTC_GetBackupRegister(uint32_t reg);                  ///< Read a backup reg back (e.g. assert the cold-boot stamp)


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_RTC_H
