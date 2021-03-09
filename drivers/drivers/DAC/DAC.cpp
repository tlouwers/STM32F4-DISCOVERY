/**
 * \file DAC.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Dac
 *
 * \brief   DAC peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/DAC
 *
 * \note    For 12-bit only using right alignment in order to be consistent with 8-bit mode.
 *          Can only switch the config if the channel is off.
 *          SetValue uses: DAC(output) = Vref * (value / (DAC(precision) + 1) ) - for 12-bit: DAC(output) = 3.3V * (value / (0xFFF + 1)) --> var = (value * (0xFFF + 1)) / 3.3V
 *          DMA is fixed: DMA1 Channel 7, Stream 5 --> PA4 (DAC Channel 1)
 *                        DMA1 Channel 7, Stream 6 --> PA5 (DAC Channel 2)
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/DAC/DAC.hpp"
#include "utility/SlimAssert/SlimAssert.h"
#include "stm32f4xx_hal_dac.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal DAC administration.
 */
Dac::Dac() :
    mInitialized(false)
{ ; }

/**
 * \brief   Destructor, stops DAC channels.
 */
Dac::~Dac()
{
    Sleep();

    SetWaveform(Channel::CHANNEL_1, nullptr, 0);
    SetWaveform(Channel::CHANNEL_2, nullptr, 0);

    mInitialized = false;
}

/**
 * \brief   Initializes the DAC.
 * \returns True if the DAC could be initialized, else false.
 */
bool Dac::Init()
{
    CheckAndEnableAHB1PeripheralClock();

    mHandle.Instance = DAC;

    if (HAL_DAC_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if DAC is initialized.
 * \returns True if DAC is initialized, else false.
 */
bool Dac::IsInit() const
{
    return mInitialized;
}

/**
 * \brief    Puts the DAC module in sleep mode.
 */
void Dac::Sleep()
{
    StopChannel(Channel::CHANNEL_1);
    StopChannel(Channel::CHANNEL_2);

    mInitialized = false;

    // ToDo: low power state, check recovery after sleep
}

/**
 * \brief   Get the handle to the peripheral.
 * \returns The handle to the peripheral.
 */
const DAC_HandleTypeDef* Dac::GetPeripheralHandle() const
{
    return &mHandle;
}

/**
 * \brief   Get the pointer to the DAC channel 1 DMA handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The DAC channel 1 DMA handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& Dac::GetDmaChannel1Handle()
{
    return mHandle.DMA_Handle1;
}

/**
 * \brief   Get the pointer to the DAC channel 2 DMA handle.
 * \details This is returned as reference-to-pointer to allow it to be changed
 *          externally, as it needs to be linked to the DMA class.
 * \returns The DAC channel 2 DMA handle as reference-to-pointer.
 */
DMA_HandleTypeDef*& Dac::GetDmaChannel2Handle()
{
    return mHandle.DMA_Handle2;
}

/**
 * \brief   Configure a channel with a given configuration.
 * \param   channel         The channel to configure.
 * \param   channelConfig   The configuration for the given channel.
 * \returns True if the channel could be configured, else false. Stops the
 *          channel before configuring it.
 */
bool Dac::ConfigureChannel(const Channel& channel, const ChannelConfig& channelConfig)
{
    if (mInitialized)
    {
        DAC_ChannelConfTypeDef chanConf = {};

        chanConf.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
        chanConf.DAC_Trigger      = GetTrigger(channelConfig.mTrigger);

        switch (channel)
        {
            case Channel::CHANNEL_1:
                if (mChannel1.mStarted)
                {
                    StopChannel(Channel::CHANNEL_1);
                }
                mChannel1.mStarted   = false;
                mChannel1.mPrecision = channelConfig.mPrecision;
                mChannel1.mTrigger   = channelConfig.mTrigger;
                if (HAL_DAC_ConfigChannel(&mHandle, &chanConf, DAC_CHANNEL_1) == HAL_OK)
                {
                    return true;
                }
                break;
            case Channel::CHANNEL_2:
                if (mChannel1.mStarted)
                {
                    StopChannel(Channel::CHANNEL_2);
                }
                mChannel2.mStarted   = false;
                mChannel2.mPrecision = channelConfig.mPrecision;
                mChannel2.mTrigger   = channelConfig.mTrigger;
                if (HAL_DAC_ConfigChannel(&mHandle, &chanConf, DAC_CHANNEL_2) == HAL_OK)
                {
                    return true;
                }
                break;
            default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
        }
    }
    return false;
}

/**
 * \brief   Configures the waveform for use with the DAC.
 * \param   channel The channel to configure the waveform for.
 * \param   values  Pointer to the buffer containing the waveform values.
 * \param   length  Length of the buffer.
 * \returns True if the waveform could be configured, else false.
 */
bool Dac::ConfigureWaveform(const Channel& channel, uint16_t* values, uint16_t length)
{
    if (values != nullptr && length > 0)
    {
        SetWaveform(channel, values, length);
        return true;
    }
    return false;
}

/**
 * \brief   Output a value with the DAC.
 * \param   channel     The channel on which to output a value.
 * \param   value       The value to output. This is in DAC counts.
 * \returns True if the value could be output, else false. Starts the channel
 *          if needed.
 */
bool Dac::SetValue(const Channel& channel, uint16_t value)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (StartChannel(Channel::CHANNEL_1))
            {
                if (HAL_DAC_SetValue(&mHandle, DAC_CHANNEL_1, GetAlignment(mChannel1.mPrecision), value) == HAL_OK)
                {
                    return true;
                }
            }
            break;
        case Channel::CHANNEL_2:
            if (StartChannel(Channel::CHANNEL_2))
            {
                if (HAL_DAC_SetValue(&mHandle, DAC_CHANNEL_2, GetAlignment(mChannel2.mPrecision), value) == HAL_OK)
                {
                    return true;
                }
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
    return false;
}

/**
 * \brief   Start the configured waveform on the DAC for the given channel.
 * \param   channel     Channel to output the configured waveform on.
 * \returns True if the waveform could be started on the given channel, else false.
 */
bool Dac::StartWaveform(const Channel& channel)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (! mChannel1.mStarted && (mWaveformChannel1.mLength > 0))
            {
                if (mHandle.DMA_Handle1 == nullptr) { return false; }

                if (! HAL_DAC_Start_DMA(&mHandle, DAC_CHANNEL_1,
                        reinterpret_cast<uint32_t*>(mWaveformChannel1.mValues), mWaveformChannel1.mLength,
                        GetAlignment(mChannel1.mPrecision)) == HAL_OK)
                {
                    mChannel1.mStarted = false;
                    return false;
                }
                mChannel1.mStarted = true;
                return true;
            }
            break;
        case Channel::CHANNEL_2:
            if (! mChannel2.mStarted && (mWaveformChannel2.mLength > 0))
            {
                if (mHandle.DMA_Handle2 == nullptr) { return false; }

                if (! HAL_DAC_Start_DMA(&mHandle, DAC_CHANNEL_2,
                        reinterpret_cast<uint32_t*>(mWaveformChannel2.mValues), mWaveformChannel2.mLength,
                        GetAlignment(mChannel2.mPrecision)) == HAL_OK)
                {
                    mChannel2.mStarted = false;
                    return false;
                }
                mChannel2.mStarted = true;
                return true;
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
    return false;
}

/**
 * \brief   Stop the configured waveform on the DAC for the given channel.
 * \param   channel     Channel to stop the configured waveform on.
 * \returns True if the waveform could be stopped for the given channel, else false.
 */
bool Dac::StopWaveform(const Channel& channel)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (mChannel1.mStarted && (mWaveformChannel1.mLength > 0))
            {
                HAL_DAC_Stop_DMA(&mHandle, DAC_CHANNEL_1);
                mChannel1.mStarted = false;
                return true;
            }
            break;
        case Channel::CHANNEL_2:
            if (mChannel2.mStarted && (mWaveformChannel2.mLength > 0))
            {
                HAL_DAC_Stop_DMA(&mHandle, DAC_CHANNEL_2);
                mChannel2.mStarted = false;
                return true;
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Check if the appropriate AHB1 peripheral clock for the DAC
 *          is enabled, if not enable it.
 */
void Dac::CheckAndEnableAHB1PeripheralClock()
{
    if (__HAL_RCC_DAC_IS_CLK_DISABLED()) { __HAL_RCC_DAC_CLK_ENABLE(); }
}

/**
 * \brief   Get the translated channel trigger value.
 * \param   trigger     The desired trigger value.
 * \returns Translated channel trigger value.
 */
uint32_t Dac::GetTrigger(const Trigger& trigger)
{
    uint32_t trigger_value = 0;

    switch (trigger)
    {
        case Trigger::TIMER_2:    { trigger_value = DAC_TRIGGER_T2_TRGO;  } break;
        case Trigger::TIMER_4:    { trigger_value = DAC_TRIGGER_T4_TRGO;  } break;
        case Trigger::TIMER_5:    { trigger_value = DAC_TRIGGER_T5_TRGO;  } break;
        case Trigger::TIMER_6:    { trigger_value = DAC_TRIGGER_T6_TRGO;  } break;
        case Trigger::TIMER_7:    { trigger_value = DAC_TRIGGER_T7_TRGO;  } break;
        case Trigger::TIMER_8:    { trigger_value = DAC_TRIGGER_T8_TRGO;  } break;
        case Trigger::EXT_LINE_9: { trigger_value = DAC_TRIGGER_EXT_IT9;  } break;
        case Trigger::SOFTWARE:   { trigger_value = DAC_TRIGGER_SOFTWARE; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return trigger_value;
}

/**
 * \brief   Get the value alignment based upon the selected presicion.
 * \param   precision   The desired precision.
 * \returns The matching value alignment.
 */
uint32_t Dac::GetAlignment(const Precision& precision)
{
    uint32_t alignment = 0;

    switch (precision)
    {
        case Precision::_8_BIT_R:  { alignment = DAC_ALIGN_8B_R;  } break;
        case Precision::_12_BIT_L: { alignment = DAC_ALIGN_12B_L; } break;
        case Precision::_12_BIT_R: { alignment = DAC_ALIGN_12B_R; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return alignment;
}

/**
 * \brief   Start the DAC channel (if not started before).
 * \param   channel     The channel to start.
 * \returns True if the channel is started, else false.
 */
bool Dac::StartChannel(const Channel& channel)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (! mChannel1.mStarted)
            {
                if (! HAL_DAC_Start(&mHandle, DAC_CHANNEL_1) == HAL_OK)
                {
                    mChannel1.mStarted = false;
                    return false;
                }
                mChannel1.mStarted = true;
            }
            break;
        case Channel::CHANNEL_2:
            if (! mChannel2.mStarted)
            {
                if (! HAL_DAC_Start(&mHandle, DAC_CHANNEL_2) == HAL_OK)
                {
                    mChannel2.mStarted = false;
                    return false;
                }
                mChannel2.mStarted = true;
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
    return false;
}

/**
 * \brief   Stop the DAC channel (if started before).
 * \param   channel     The channel to stop.
 * \returns True if the channel is stopped, else false.
 */
bool Dac::StopChannel(const Channel& channel)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (mChannel1.mStarted)
            {
                if (mWaveformChannel1.mLength == 0)
                {
                    HAL_DAC_Stop(&mHandle, DAC_CHANNEL_1);
                } else {
                    HAL_DAC_Stop_DMA(&mHandle, DAC_CHANNEL_1);
                }
                mChannel1.mStarted = false;
                return true;
            }
            break;
        case Channel::CHANNEL_2:
            if (mChannel2.mStarted)
            {
                if (mWaveformChannel2.mLength == 0)
                {
                    HAL_DAC_Stop(&mHandle, DAC_CHANNEL_2);
                } else {
                    HAL_DAC_Stop_DMA(&mHandle, DAC_CHANNEL_2);
                }
                mChannel2.mStarted = false;
                return true;
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
    return false;
}

/**
 * \brief   For the given channel, set the waveform administration.
 * \param   channel     The channel for which to set the waveform.
 * \param   values      Pointer to the buffer containing the waveform values.
 * \param   length      Length of the buffer.
 */
void Dac::SetWaveform(const Channel& channel, uint16_t* values, uint16_t length)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            mWaveformChannel1.mValues = values;
            mWaveformChannel1.mLength = length;
            mWaveformChannel1.mIndex  = 0;
            break;
        case Channel::CHANNEL_2:
            mWaveformChannel2.mValues = values;
            mWaveformChannel2.mLength = length;
            mWaveformChannel2.mIndex  = 0;
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
}
