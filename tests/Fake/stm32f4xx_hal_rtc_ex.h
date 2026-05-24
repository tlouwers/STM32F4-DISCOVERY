/**
 * \file    stm32f4xx_hal_rtc_ex.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL RTC extended surface for the
 *          unit-test build. Provides the backup-register read/write entry
 *          points the Rtc driver uses for its cold/warm-boot magic detection.
 *
 * \details The backup registers are modelled as fake storage shared with
 *          stm32f4xx_hal_rtc.cpp; BKUPWrite stores and BKUPRead returns the
 *          stored word, so the driver's "magic absent -> cold boot -> seed +
 *          stamp" sequence is observable. The register index macros mirror the
 *          real HAL (BKP_DR0 = 0).
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_RTC_EX_H
#define __STM32F4xx_HAL_RTC_EX_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rtc.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Backup-register indices (mirror the real HAL)                        */
/************************************************************************/
#define RTC_BKP_DR0     ((uint32_t)0x00000000U)
#define RTC_BKP_DR1     ((uint32_t)0x00000001U)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_rtc.cpp)          */
/************************************************************************/
uint32_t HAL_RTCEx_BKUPRead(RTC_HandleTypeDef* hrtc, uint32_t BackupRegister);
void     HAL_RTCEx_BKUPWrite(RTC_HandleTypeDef* hrtc, uint32_t BackupRegister, uint32_t Data);


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_RTC_EX_H
