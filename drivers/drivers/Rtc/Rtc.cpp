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
 * \details This assumes the RTC is configured and running, using the LSE (slow
 *          crystal, 32768 Hz). Most likely this is done in Board::InitClock().
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/drivers/Rtc
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
#include "stm32f4xx_hal_rtc.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static constexpr uint16_t YEAR_OFFSET = 2000;


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
 */
bool Rtc::Init(const IConfig& config)
{
    const Config& cfg = reinterpret_cast<const Config&>(config);

    EnablePeripheralClock(cfg.mClockSource);

    mHandle.Init.HourFormat     = RTC_HOURFORMAT_24;    // Not using the 12-hout format
    mHandle.Init.AsynchPrediv   = 127;                  // If using 32768 Hz crystal, these dividers result in 1 Hz tick
    mHandle.Init.SynchPrediv    = 255;
    mHandle.Init.OutPut         = RTC_OUTPUT_DISABLE;
    mHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    mHandle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&mHandle) == HAL_OK)
    {
        // Reset date
        RTC_TimeTypeDef sTime = {};
        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;
        if (HAL_RTC_SetTime(&mHandle, &sTime, RTC_FORMAT_BCD) == HAL_OK)
        {
            // Reset time
            RTC_DateTypeDef sDate = {};
            sDate.WeekDay = RTC_WEEKDAY_MONDAY;
            sDate.Month   = RTC_MONTH_JANUARY;
            sDate.Date    = 0x01;
            sDate.Year    = 0x00;
            if (HAL_RTC_SetDate(&mHandle, &sDate, RTC_FORMAT_BCD) == HAL_OK)
            {
                mInitialized = true;
                return true;
            }
        }
    }
    return false;
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
    mInitialized = false;

    if (HAL_RTC_DeInit(&mHandle) == HAL_OK)
    {
        DisablePeripheralClock();
        return true;
    }
    return false;
}

/**
 * \brief   Set the date and time to the indicated values.
 * \param   dateTime    The date and time to set.
 * \returns True if the data and time could be set, else false.
 * \note    Year must be equal or larger than YEAR_OFFSET.
 */
bool Rtc::SetDateTime(const DateTime& dateTime)
{
    if (!mInitialized)               { return false; }
    if (dateTime.year < YEAR_OFFSET) { return false; }

    RTC_TimeTypeDef sTime = {};
    sTime.Hours   = dateTime.hour;
    sTime.Minutes = dateTime.minute;
    sTime.Seconds = dateTime.second;
    if (HAL_RTC_SetTime(&mHandle, &sTime, RTC_FORMAT_BCD) == HAL_OK)
    {
        RTC_DateTypeDef sDate = {};
        sDate.Year  = dateTime.year - YEAR_OFFSET;
        sDate.Month = dateTime.month;
        sDate.Date  = dateTime.day;
        return (HAL_RTC_SetDate(&mHandle, &sDate, RTC_FORMAT_BCD) == HAL_OK);
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
 * \brief   Enable the appropriate peripheral clock for the RTC.
 * \param   clockSource     The clock source to be configured.
 * \note    Asserts if not a valid cock source provided.
 */
void Rtc::EnablePeripheralClock(const ClockSource& clockSource)
{
    // Configure the clock source first
    switch (clockSource)
    {
        case ClockSource::LSI: { __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSI);      } break;
        case ClockSource::LSE: { __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);      } break;
        case ClockSource::HSE: { __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_HSE_DIV8); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    // Then enable the RTC clock
    __HAL_RCC_RTC_ENABLE();
}

/**
 * \brief   Disable the peripheral clock for the RTC.
 */
void Rtc::DisablePeripheralClock()
{
    __HAL_RCC_RTC_DISABLE();
}
