/**
 * \file    stm32f4xx_hal_pwr.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL PWR surface for the unit-test build.
 *          The Rtc driver only needs to unlock backup-domain write access
 *          before touching RCC_BDCR; on the native build there is no backup
 *          domain to protect, so both entry points are no-ops.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_PWR_H
#define __STM32F4xx_HAL_PWR_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Backup-domain access (mirror the real HAL -- no-ops natively)        */
/************************************************************************/
/* On hardware these gate writes to the backup domain (RCC_BDCR); the native
   build has no such protection, so enabling the PWR clock / backup access is a
   no-op. Function-like macros keep the driver's call sites (`...();`) valid. */
#define __HAL_RCC_PWR_CLK_ENABLE()    do { } while(0)
#define HAL_PWR_EnableBkUpAccess()    do { } while(0)


#endif  // __STM32F4xx_HAL_PWR_H
