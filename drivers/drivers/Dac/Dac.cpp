/**
 * \file Dac.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Dac
 *
 * \brief   Dac peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/Dac
 *
 * \note    For 12-bit only using right alignment in order to be consistent with 8-bit mode.
 *          Can only switch the config if the channel is off.
 *          SetValue uses: Dac(output) = Vref * (value / (Dac(precision) + 1) ) - for 12-bit: Dac(output) = 3.3V * (value / (0xFFF + 1)) --> var = (value * (0xFFF + 1)) / 3.3V
 *          DMA is fixed: DMA1 Channel 7, Stream 5 --> PA4 (Dac Channel 1)
 *                        DMA1 Channel 7, Stream 6 --> PA5 (Dac Channel 2)
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Dac/Dac.hpp"
#include "utility/Assert/Assert.h"
#include "stm32f4xx_hal_dac.h"


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal Dac administration.
 */
Dac::Dac() :
    mInitialized(false)
{ ; }

/**
 * \brief   Destructor, stops Dac channels.
 */
Dac::~Dac()
{
    Sleep();
}

/**
 * \brief   Initializes the Dac.
 * \returns True if the Dac could be initialized, else false.
 */
bool Dac::Init()
{
    CheckAndEnablePeripheralClock();

    mHandle.Instance = DAC;

    if (HAL_DAC_Init(&mHandle) == HAL_OK)
    {
        mInitialized = true;
        return true;
    }
    return false;
}

/**
 * \brief   Indicate if Dac is initialized.
 * \returns True if Dac is initialized, else false.
 */
bool Dac::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the Dac module in sleep mode.
 * \details Stops output on channel(s).
 * \returns True if Dac module could be put in sleep mode, else false.
 */
bool Dac::Sleep()
{
    StopChannel(Channel::CHANNEL_1);
    StopChannel(Channel::CHANNEL_2);

    SetWaveform(Channel::CHANNEL_1, nullptr, 0);
    SetWaveform(Channel::CHANNEL_2, nullptr, 0);

    if (HAL_DAC_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    CheckAndDisablePeripheralClock();
    return true;
}

/**
 * \brief   Link a configured DMA stream into the Dac's channel-1 or channel-2 slot.
 * \param   channel The Dac channel whose DMA slot to wire (CHANNEL_1 → DMA_Handle1,
 *                  CHANNEL_2 → DMA_Handle2).
 * \param   dma     A DMA object that has been Configure()'d with Direction
 *                  MemoryToPeripheral (both Dac channels are outputs).
 * \returns True if the DMA was linked, false if dma was not configured or
 *          its Direction is not MemoryToPeripheral.
 */
bool Dac::LinkDma(const Channel& channel, DMA& dma)
{
    if (!dma.IsConfigured())                                            { return false; }
    if (dma.GetDirection() != DMA::Direction::MemoryToPeripheral)       { return false; }

    switch (channel)
    {
        case Channel::CHANNEL_1:
            __HAL_LINKDMA(&mHandle, DMA_Handle1, *dma.Handle());
            return true;
        case Channel::CHANNEL_2:
            __HAL_LINKDMA(&mHandle, DMA_Handle2, *dma.Handle());
            return true;
        default:
            ASSERT(false);
            return false;
    }
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
                StopChannel(Channel::CHANNEL_1);
                if (HAL_DAC_ConfigChannel(&mHandle, &chanConf, DAC_CHANNEL_1) == HAL_OK)
                {
                    mChannel1.mPrecision = channelConfig.mPrecision;
                    mChannel1.mTrigger   = channelConfig.mTrigger;
                    return true;
                }
                break;
            case Channel::CHANNEL_2:
                StopChannel(Channel::CHANNEL_2);
                if (HAL_DAC_ConfigChannel(&mHandle, &chanConf, DAC_CHANNEL_2) == HAL_OK)
                {
                    mChannel2.mPrecision = channelConfig.mPrecision;
                    mChannel2.mTrigger   = channelConfig.mTrigger;
                    return true;
                }
                break;
            default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
        }
    }
    return false;
}

/**
 * \brief   Configures the waveform for use with the Dac.
 * \param   channel The channel to configure the waveform for.
 * \param   values  Pointer to the buffer containing the waveform values.
 * \param   length  Length of the buffer.
 * \returns True if the waveform could be configured, else false.
 */
bool Dac::ConfigureWaveform(const Channel& channel, const uint16_t* values, uint16_t length)
{
    EXPECT(values);
    EXPECT(length > 0);

    if (values == nullptr) { return false; }
    if (length == 0)       { return false; }
    if (!mInitialized)     { return false; }

    SetWaveform(channel, values, length);
    return true;
}

/**
 * \brief   Output a value with the Dac.
 * \param   channel     The channel on which to output a value.
 * \param   value       The value to output. This is in Dac counts.
 * \returns True if the value could be output, else false. Starts the channel
 *          if needed.
 */
bool Dac::SetValue(const Channel& channel, uint16_t value)
{
    if (!mInitialized) { return false; }

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
 * \brief   Start the configured waveform on the Dac for the given channel.
 * \param   channel     Channel to output the configured waveform on.
 * \returns True if the waveform could be started on the given channel, else false.
 */
bool Dac::StartWaveform(const Channel& channel)
{
    if (!mInitialized) { return false; }

    switch (channel)
    {
        case Channel::CHANNEL_1:
            if ((mHandle.DMA_Handle1 == nullptr) || (mWaveformChannel1.mLength == 0)) { return false; }

            if (! mChannel1.mStarted)
            {
                if (HAL_DAC_Start_DMA(&mHandle, DAC_CHANNEL_1,
                        reinterpret_cast<uint32_t*>(mWaveformChannel1.mValues), mWaveformChannel1.mLength,
                        GetAlignment(mChannel1.mPrecision)) == HAL_OK)
                {
                    mChannel1.mStarted = true;
                    return true;
                }
            }
            break;
        case Channel::CHANNEL_2:
            if ((mHandle.DMA_Handle2 == nullptr) || (mWaveformChannel2.mLength == 0)) { return false; }

            if (! mChannel2.mStarted)
            {
                if (HAL_DAC_Start_DMA(&mHandle, DAC_CHANNEL_2,
                        reinterpret_cast<uint32_t*>(mWaveformChannel2.mValues), mWaveformChannel2.mLength,
                        GetAlignment(mChannel2.mPrecision)) == HAL_OK)
                {
                    mChannel2.mStarted = true;
                    return true;
                }
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
    return false;
}

/**
 * \brief   Stop the configured waveform on the Dac for the given channel.
 * \param   channel     Channel to stop the configured waveform on.
 * \returns True if the waveform could be stopped for the given channel, else false.
 */
bool Dac::StopWaveform(const Channel& channel)
{
    if (!mInitialized) { return false; }

    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (mChannel1.mStarted)
            {
                HAL_DAC_Stop_DMA(&mHandle, DAC_CHANNEL_1);
                mChannel1.mStarted = false;
                return true;
            }
            break;
        case Channel::CHANNEL_2:
            if (mChannel2.mStarted)
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
 * \brief   Enable the peripheral clock for the Dac.
 */
void Dac::CheckAndEnablePeripheralClock()
{
    __HAL_RCC_DAC_CLK_ENABLE();
}

/**
 * \brief   Disable the peripheral clock for the Dac.
 */
void Dac::CheckAndDisablePeripheralClock()
{
    __HAL_RCC_DAC_CLK_DISABLE();
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
        case Trigger::NONE:       { trigger_value = DAC_TRIGGER_NONE;     } break;
        case Trigger::TIMER_2:    { trigger_value = DAC_TRIGGER_T2_TRGO;  } break;
        case Trigger::TIMER_4:    { trigger_value = DAC_TRIGGER_T4_TRGO;  } break;
        case Trigger::TIMER_5:    { trigger_value = DAC_TRIGGER_T5_TRGO;  } break;
        case Trigger::TIMER_6:    { trigger_value = DAC_TRIGGER_T6_TRGO;  } break;
        case Trigger::TIMER_7:    { trigger_value = DAC_TRIGGER_T7_TRGO;  } break;
        case Trigger::TIMER_8:    { trigger_value = DAC_TRIGGER_T8_TRGO;  } break;
        case Trigger::EXT_LINE_9: { trigger_value = DAC_TRIGGER_EXT_IT9;  } break;
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
 * \brief   Start the Dac channel (if not started before).
 * \param   channel     The channel to start.
 * \returns True if the channel is started, else false.
 */
bool Dac::StartChannel(const Channel& channel)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            if (mChannel1.mStarted) { return true; }    // Already started

            if (HAL_DAC_Start(&mHandle, DAC_CHANNEL_1) == HAL_OK)
            {
                mChannel1.mStarted = true;
                return true;
            }
            break;
        case Channel::CHANNEL_2:
            if (mChannel2.mStarted) { return true; }    // Already started

            if (HAL_DAC_Start(&mHandle, DAC_CHANNEL_2) == HAL_OK)
            {
                mChannel2.mStarted = true;
                return true;
            }
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
    return false;
}

/**
 * \brief   Stop the Dac channel (if started before).
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
void Dac::SetWaveform(const Channel& channel, const uint16_t* values, uint16_t length)
{
    switch (channel)
    {
        case Channel::CHANNEL_1:
            mWaveformChannel1.mValues = const_cast<uint16_t*>(values);
            mWaveformChannel1.mLength = length;
            mWaveformChannel1.mIndex  = 0;
            break;
        case Channel::CHANNEL_2:
            mWaveformChannel2.mValues = const_cast<uint16_t*>(values);
            mWaveformChannel2.mLength = length;
            mWaveformChannel2.mIndex  = 0;
            break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    };
}
