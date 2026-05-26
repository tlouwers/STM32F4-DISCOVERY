/**
 * \file    stm32f4xx_hal_rtc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake implementation of the STM32F4 HAL RTC surface. Provides
 *          the single RTC instance (backed by real storage so the driver's
 *          handle is non-null), fake time/date registers, fake backup
 *          registers, and the HAL_RTC_* / HAL_RTCEx_BKUP* bodies. C linkage is
 *          preserved on every symbol the driver under test links against.
 *
 * \details Each entry point returns its own controllable result (HAL_OK by
 *          default) so the driver's per-operation failure branches can be
 *          exercised in isolation. SetTime/SetDate store into the fake
 *          registers (only on success); GetTime/GetDate read them back, so a
 *          set/get round-trip through the driver is observable. The backup
 *          registers persist BKUPWrite values for BKUPRead, modelling the
 *          cold/warm-boot magic detection in Rtc::Init.
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
extern "C" {
#include "stm32f4xx_hal_rtc.h"
#include "stm32f4xx_hal_rtc_ex.h"
}


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
namespace {

constexpr uint32_t NUM_BACKUP_REGISTERS = 20;   // F407 has RTC_BKP_DR0..DR19

RTC_TypeDef s_rtc = {};

RTC_TimeTypeDef s_time = {};
RTC_DateTypeDef s_date = {};
uint32_t        s_bkup[NUM_BACKUP_REGISTERS] = {};

HAL_StatusTypeDef s_init_result     = HAL_OK;
HAL_StatusTypeDef s_deinit_result   = HAL_OK;
HAL_StatusTypeDef s_set_time_result = HAL_OK;
HAL_StatusTypeDef s_get_time_result = HAL_OK;
HAL_StatusTypeDef s_set_date_result = HAL_OK;
HAL_StatusTypeDef s_get_date_result = HAL_OK;

} // namespace


/************************************************************************/
/* RTC instance (C linkage to match the driver's view)                  */
/************************************************************************/
extern "C" RTC_TypeDef* const RTC = &s_rtc;


/************************************************************************/
/* Fake HAL entry points                                                */
/************************************************************************/
extern "C" HAL_StatusTypeDef HAL_RTC_Init(RTC_HandleTypeDef* /*hrtc*/)   { return s_init_result; }
extern "C" HAL_StatusTypeDef HAL_RTC_DeInit(RTC_HandleTypeDef* /*hrtc*/) { return s_deinit_result; }

extern "C" HAL_StatusTypeDef HAL_RTC_SetTime(RTC_HandleTypeDef* /*hrtc*/, RTC_TimeTypeDef* sTime, uint32_t /*Format*/)
{
    if (s_set_time_result == HAL_OK) { s_time = *sTime; }
    return s_set_time_result;
}

extern "C" HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef* /*hrtc*/, RTC_TimeTypeDef* sTime, uint32_t /*Format*/)
{
    if (s_get_time_result == HAL_OK) { *sTime = s_time; }
    return s_get_time_result;
}

extern "C" HAL_StatusTypeDef HAL_RTC_SetDate(RTC_HandleTypeDef* /*hrtc*/, RTC_DateTypeDef* sDate, uint32_t /*Format*/)
{
    if (s_set_date_result == HAL_OK) { s_date = *sDate; }
    return s_set_date_result;
}

extern "C" HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef* /*hrtc*/, RTC_DateTypeDef* sDate, uint32_t /*Format*/)
{
    if (s_get_date_result == HAL_OK) { *sDate = s_date; }
    return s_get_date_result;
}


/************************************************************************/
/* Fake backup-register surface                                         */
/************************************************************************/
extern "C" uint32_t HAL_RTCEx_BKUPRead(RTC_HandleTypeDef* /*hrtc*/, uint32_t BackupRegister)
{
    return (BackupRegister < NUM_BACKUP_REGISTERS) ? s_bkup[BackupRegister] : 0U;
}

extern "C" void HAL_RTCEx_BKUPWrite(RTC_HandleTypeDef* /*hrtc*/, uint32_t BackupRegister, uint32_t Data)
{
    if (BackupRegister < NUM_BACKUP_REGISTERS) { s_bkup[BackupRegister] = Data; }
}


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
extern "C" void FakeRTC_Reset(void)
{
    s_time = RTC_TimeTypeDef{};
    s_date = RTC_DateTypeDef{};
    for (uint32_t i = 0; i < NUM_BACKUP_REGISTERS; ++i) { s_bkup[i] = 0U; }

    s_init_result     = HAL_OK;
    s_deinit_result   = HAL_OK;
    s_set_time_result = HAL_OK;
    s_get_time_result = HAL_OK;
    s_set_date_result = HAL_OK;
    s_get_date_result = HAL_OK;
}

extern "C" void FakeRTC_SetInitResult(HAL_StatusTypeDef result)    { s_init_result     = result; }
extern "C" void FakeRTC_SetDeInitResult(HAL_StatusTypeDef result)  { s_deinit_result   = result; }
extern "C" void FakeRTC_SetSetTimeResult(HAL_StatusTypeDef result) { s_set_time_result = result; }
extern "C" void FakeRTC_SetGetTimeResult(HAL_StatusTypeDef result) { s_get_time_result = result; }
extern "C" void FakeRTC_SetSetDateResult(HAL_StatusTypeDef result) { s_set_date_result = result; }
extern "C" void FakeRTC_SetGetDateResult(HAL_StatusTypeDef result) { s_get_date_result = result; }

extern "C" void FakeRTC_SetBackupRegister(uint32_t reg, uint32_t value)
{
    if (reg < NUM_BACKUP_REGISTERS) { s_bkup[reg] = value; }
}

extern "C" uint32_t FakeRTC_GetBackupRegister(uint32_t reg)
{
    return (reg < NUM_BACKUP_REGISTERS) ? s_bkup[reg] : 0U;
}
