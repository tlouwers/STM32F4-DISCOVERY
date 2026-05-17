/**
 * \file    Rtc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Rtc
 *
 * \brief   Rtc peripheral driver class.
 *
 * \details This assumes the Rtc is configured and running, using the LSE (slow
 *          crystal, 32768 Hz). Most likely this is done in Board::InitClock().
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/Rtc
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Rtc/Rtc.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_rtc.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static constexpr uint16_t YEAR_OFFSET = 2000;
static constexpr uint16_t YEAR_MAX    = 2099;   // RTC year register is BCD 00..99


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor.
 */
Rtc::Rtc() :
    mInitialized(false)
{
    mHandle.Instance = RTC;
}

/**
 * \brief   Destructor.
 */
Rtc::~Rtc()
{
    Sleep();
}

/**
 * \brief   Initializes the Rtc instance.
 * \returns True if Rtc could be initialized, else false.
 * \note    Init does NOT touch the RTC time/date registers, so a battery-
 *          backed time survives a warm boot. Call SetDateTime explicitly
 *          to seed the clock.
 * \note    RCC_BDCR.RTCSEL is write-once after a backup-domain reset (RM0090
 *          §6.3.20). If a previous boot configured a different ClockSource,
 *          this call cannot change it -- the existing source stays in
 *          effect. Workaround would be a backup-domain reset, which would
 *          also wipe the preserved time.
 */
bool Rtc::Init(const IConfig& config)
{
    EXPECT(config.ConfigId() == Config::Id());
    if (config.ConfigId() != Config::Id()) { return false; }

    const Config& cfg = static_cast<const Config&>(config);

    EnablePeripheralClock(cfg.mClockSource);

    mHandle.Init.HourFormat     = RTC_HOURFORMAT_24;    // Not using the 12-hour format
    mHandle.Init.AsynchPrediv   = 127;                  // If using 32768 Hz crystal, these dividers result in 1 Hz tick
    mHandle.Init.SynchPrediv    = 255;
    mHandle.Init.OutPut         = RTC_OUTPUT_DISABLE;
    mHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    mHandle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&mHandle) != HAL_OK) { return false; }

    mInitialized = true;
    return true;
}

/**
 * \brief   Indicate if Rtc is initialized.
 * \returns True if Rtc is initialized, else false.
 */
bool Rtc::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the Rtc module in sleep mode.
 * \returns True if Rtc module could be put in sleep mode, else false.
 */
bool Rtc::Sleep()
{
    if (HAL_RTC_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    DisablePeripheralClock();
    return true;
}

/**
 * \brief   Set the date and time to the indicated values.
 * \param   dateTime    The date and time to set.
 * \returns True if the date and time could be set, else false.
 * \note    Fields are range-checked per IRtc::DateTime: year [2000..2099],
 *          month [1..12], day [1..31], hour [0..23], minute / second
 *          [0..59]. Day-of-month is not cross-checked against month, the
 *          RTC peripheral itself handles invalid calendar combinations.
 */
bool Rtc::SetDateTime(const DateTime& dateTime)
{
    EXPECT(dateTime.year   >= YEAR_OFFSET);
    EXPECT(dateTime.year   <= YEAR_MAX);
    EXPECT(dateTime.month  >= 1 && dateTime.month  <= 12);
    EXPECT(dateTime.day    >= 1 && dateTime.day    <= 31);
    EXPECT(dateTime.hour   <= 23);
    EXPECT(dateTime.minute <= 59);
    EXPECT(dateTime.second <= 59);

    if (!mInitialized)                                 { return false; }
    if (dateTime.year   < YEAR_OFFSET)                 { return false; }
    if (dateTime.year   > YEAR_MAX)                    { return false; }
    if (dateTime.month  < 1 || dateTime.month  > 12)   { return false; }
    if (dateTime.day    < 1 || dateTime.day    > 31)   { return false; }
    if (dateTime.hour   > 23)                          { return false; }
    if (dateTime.minute > 59)                          { return false; }
    if (dateTime.second > 59)                          { return false; }

    RTC_TimeTypeDef sTime = {};
    sTime.Hours   = dateTime.hour;
    sTime.Minutes = dateTime.minute;
    sTime.Seconds = dateTime.second;
    if (HAL_RTC_SetTime(&mHandle, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
        RTC_DateTypeDef sDate = {};
        sDate.Year  = dateTime.year - YEAR_OFFSET;
        sDate.Month = dateTime.month;
        sDate.Date  = dateTime.day;
        return (HAL_RTC_SetDate(&mHandle, &sDate, RTC_FORMAT_BIN) == HAL_OK);
    }
    return false;
}

/**
 * \brief   Get the date and time.
 * \param   dateTime    Structure to store the date and time.
 * \returns True if the data and time could be get, else false.
 * \note    Year will always be larger than YEAR_OFFSET.
 */
bool Rtc::GetDateTime(DateTime &dateTime)
{
    if (!mInitialized) { return false; }

    RTC_TimeTypeDef sTime = {};
    if (HAL_RTC_GetTime(&mHandle, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
        RTC_DateTypeDef sDate = {};
        if (HAL_RTC_GetDate(&mHandle, &sDate, RTC_FORMAT_BIN) == HAL_OK)
        {
            dateTime.year   = YEAR_OFFSET + sDate.Year;
            dateTime.month  = sDate.Month;
            dateTime.day    = sDate.Date;
            dateTime.hour   = sTime.Hours;
            dateTime.minute = sTime.Minutes;
            dateTime.second = sTime.Seconds;
            return true;
        }
    }
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Enable the appropriate peripheral clock for the Rtc.
 * \param   clockSource     The clock source to be configured.
 * \note    RCC_BDCR (which holds RTCSEL and RTCEN) is in the backup
 *          domain; writes are silently rejected unless PWR_CR.DBP is
 *          set first. Enable PWR clock + backup-domain access before
 *          touching RCC_BDCR.
 * \note    Asserts if not a valid clock source provided.
 */
void Rtc::EnablePeripheralClock(const ClockSource& clockSource)
{
    // Allow writes to RCC_BDCR (otherwise RTCSEL/RTCEN writes silently fail).
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    // Configure the clock source first
    switch (clockSource)
    {
        case ClockSource::LSI: { __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);      } break;
        case ClockSource::LSE: { __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);      } break;
        case ClockSource::HSE: { __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_HSE_DIV8); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    // Then enable the Rtc clock
    __HAL_RCC_RTC_ENABLE();
}

/**
 * \brief   Disable the peripheral clock for the Rtc.
 */
void Rtc::DisablePeripheralClock()
{
    __HAL_RCC_RTC_DISABLE();
}
