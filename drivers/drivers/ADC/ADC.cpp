/**
 * \file    ADC.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Adc
 *
 * \brief   ADC peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/ADC
 *
 * \note    Using right alignment only to be consistent with all resolutions.
 *          GetValue uses: ADC(input) = value * (Vref / (ADC(resolution) + 1) ) - for 12-bit: ADC(input) = value * (3.3V / (0xFFF + 1) --> var = (value * (0xFFF + 1)) / 3.3V
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/ADC/ADC.hpp"
#include "utility/SlimAssert/SlimAssert.h"
#include "stm32f4xx_hal_adc.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static ADCCallbacks adc1_callbacks {};
static ADCCallbacks adc2_callbacks {};
static ADCCallbacks adc3_callbacks {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackIRQ, if configured.
 * \param   adc_callbacks   Structure containing the callbackIRQ to call.
 */
static void CallbackIRQ(const ADCCallbacks& adc_callbacks)
{
    if (adc_callbacks.callbackIRQ)
    {
        adc_callbacks.callbackIRQ();
    }
}

/**
 * \brief   Call the callbackEndOfConversion, if configured.
 * \param   adc_callbacks   Structure containing the callbackEndOfConversion to call.
 */
static void CallbackEndOfConversion(const ADCCallbacks& adc_callbacks, uint16_t value)
{
    if (adc_callbacks.callbackEndOfConversion)
    {
        adc_callbacks.callbackEndOfConversion(value);
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal ADC administration.
 */
Adc::Adc(const ADCInstance& instance) :
    mInstance(instance),
    mADCCallbacks( (instance == ADCInstance::ADC_1) ? (adc1_callbacks) : ( (instance == ADCInstance::ADC_2) ? (adc2_callbacks) : (adc3_callbacks) ) ),
    mInitialized(false)
{
    SetInstance(instance);

    mADCCallbacks.callbackIRQ = [this]() { this->CallbackIRQ(); };
}

/**
 * \brief   Destructor, stops ADC channels.
 */
Adc::~Adc()
{
    Sleep();

    mInitialized = false;
}

/**
 * \brief   Initializes the ADC instance with the given configuration.
 * \param   config  The configuration for the ADC instance to use.
 * \returns True if the configuration could be applied, else false.
 */
bool Adc::Init(const Config& config)
{
    CheckAndEnableAHB2PeripheralClock(mInstance);

    mHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
    mHandle.Init.Resolution            = GetResolution(config.mResolution);
    mHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    mHandle.Init.ScanConvMode          = DISABLE;
    mHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    mHandle.Init.ContinuousConvMode    = DISABLE;
    mHandle.Init.NbrOfConversion       = 1;
    mHandle.Init.DiscontinuousConvMode = DISABLE;
    mHandle.Init.NbrOfDiscConversion   = 0;
    mHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    mHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    mHandle.Init.DMAContinuousRequests = DISABLE;

    if (HAL_ADC_Init(&mHandle) == HAL_OK)
    {
        SetIRQn(ADC_IRQn, config.mInterruptPriority, 0);

        // Configure channel
        ADC_ChannelConfTypeDef adcChannelConfig = {};
        adcChannelConfig.Channel      = GetChannel(config.mChannel);
        adcChannelConfig.Offset       = 0;
        adcChannelConfig.Rank         = 1;
        adcChannelConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

        if (HAL_ADC_ConfigChannel(&mHandle, &adcChannelConfig) == HAL_OK)
        {
            mInitialized = true;
            return true;
        }
    }
    return false;
}

/**
 * \brief   Indicate if ADC is initialized.
 * \returns True if ADC is initialized, else false.
 */
bool Adc::IsInit() const
{
    return mInitialized;
}

/**
 * \brief    Puts the ADC module in sleep mode.
 */
void Adc::Sleep()
{
    // Disable interrupts
    HAL_NVIC_DisableIRQ( ADC_IRQn );

    HAL_ADC_Stop(&mHandle);

    mInitialized = false;

    // ToDo: low power state, check recovery after sleep
}

/**
 * \brief   Sample a value with the ADC from input.
 * \param   value   Variable to store the sampled value into.
 * \returns True if sampling was succesful, else false.
 */
bool Adc::GetValue(uint16_t& value)
{
    if (!mInitialized) { return false; }

    if (HAL_ADC_Start(&mHandle) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&mHandle, HAL_MAX_DELAY) == HAL_OK)
        {
            value = static_cast<uint16_t>(HAL_ADC_GetValue(&mHandle));

            if (HAL_ADC_Stop(&mHandle) == HAL_OK)
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * \brief   Sample a value with the ADC from input using interrupts.
 * \param   handler     Callback to call when sampling completed.
 * \returns True if the sampling could be started, else false.
 */
bool Adc::GetValueInterrupt(const std::function<void(uint16_t)>& handler)
{
    if (!mInitialized) { return false; }

    mADCCallbacks.callbackEndOfConversion = handler;

    if (HAL_ADC_Start_IT(&mHandle) == HAL_OK)
    {
        return true;
    }
    return false;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the ADC instance into internal administration.
 * \param   instance    The ADC instance to use.
 * \note    Asserts if the ADC instance is invalid.
 */
void Adc::SetInstance(const ADCInstance& instance)
{
    switch (instance)
    {
        case ADCInstance::ADC_1: mHandle.Instance = ADC1; break;
        case ADCInstance::ADC_2: mHandle.Instance = ADC2; break;
        case ADCInstance::ADC_3: mHandle.Instance = ADC3; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Check if the appropriate AHB2 peripheral clock for the ADC
 *          instance is enabled, if not enable it.
 * \param   instance    The ADC instance to enable the clock for.
 * \note    Asserts if not a valid ADC instance provided.
 */
void Adc::CheckAndEnableAHB2PeripheralClock(const ADCInstance& instance)
{
    switch (instance)
    {
        case ADCInstance::ADC_1: if (__HAL_RCC_ADC1_IS_CLK_DISABLED()) { __HAL_RCC_ADC1_CLK_ENABLE(); } break;
        case ADCInstance::ADC_2: if (__HAL_RCC_ADC2_IS_CLK_DISABLED()) { __HAL_RCC_ADC2_CLK_ENABLE(); } break;
        case ADCInstance::ADC_3: if (__HAL_RCC_ADC3_IS_CLK_DISABLED()) { __HAL_RCC_ADC3_CLK_ENABLE(); } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Get the translated channel.
 * \param   channel     The desired channel.
 * \returns Translated channel value.
 */
uint32_t Adc::GetChannel(const Channel& channel)
{
    uint32_t channel_value = ADC_CHANNEL_0;

    switch (channel)
    {
        case Channel::CHANNEL_0:  { channel_value = ADC_CHANNEL_0;  } break;
        case Channel::CHANNEL_1:  { channel_value = ADC_CHANNEL_1;  } break;
        case Channel::CHANNEL_2:  { channel_value = ADC_CHANNEL_2;  } break;
        case Channel::CHANNEL_3:  { channel_value = ADC_CHANNEL_3;  } break;
        case Channel::CHANNEL_4:  { channel_value = ADC_CHANNEL_4;  } break;
        case Channel::CHANNEL_5:  { channel_value = ADC_CHANNEL_5;  } break;
        case Channel::CHANNEL_6:  { channel_value = ADC_CHANNEL_6;  } break;
        case Channel::CHANNEL_7:  { channel_value = ADC_CHANNEL_7;  } break;
        case Channel::CHANNEL_8:  { channel_value = ADC_CHANNEL_8;  } break;
        case Channel::CHANNEL_9:  { channel_value = ADC_CHANNEL_9;  } break;
        case Channel::CHANNEL_10: { channel_value = ADC_CHANNEL_10; } break;
        case Channel::CHANNEL_11: { channel_value = ADC_CHANNEL_11; } break;
        case Channel::CHANNEL_12: { channel_value = ADC_CHANNEL_12; } break;
        case Channel::CHANNEL_13: { channel_value = ADC_CHANNEL_13; } break;
        case Channel::CHANNEL_14: { channel_value = ADC_CHANNEL_14; } break;
        case Channel::CHANNEL_15: { channel_value = ADC_CHANNEL_15; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return channel_value;
}

/**
 * \brief   Get the translated resolution value.
 * \param   resolution  The desired resolution value.
 * \returns Translated resolution value.
 */
uint32_t Adc::GetResolution(const Resolution& resolution)
{
    uint32_t resolution_value = ADC_RESOLUTION_12B;

    switch (resolution)
    {
        case Resolution::_6_BIT:  { resolution_value = ADC_RESOLUTION_6B;  } break;
        case Resolution::_8_BIT:  { resolution_value = ADC_RESOLUTION_8B;  } break;
        case Resolution::_10_BIT: { resolution_value = ADC_RESOLUTION_10B; } break;
        case Resolution::_12_BIT: { resolution_value = ADC_RESOLUTION_12B; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return resolution_value;
}

/**
 * \brief   Lower level configuration for the ADC interrupts.
 * \param   type        IRQn External interrupt number.
 * \param   preemptPrio The preemption priority for the IRQn channel.
 * \param   subPrio     The subpriority level for the IRQ channel.
 */
void Adc::SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio)
{
    HAL_NVIC_DisableIRQ(type);
    HAL_NVIC_ClearPendingIRQ(type);
    HAL_NVIC_SetPriority(type, preemptPrio, subPrio);
    HAL_NVIC_EnableIRQ(type);
}

/**
 * \brief   Generic ADC IRQ callback. Will propagate other interrupts.
 */
void Adc::CallbackIRQ() const
{
    HAL_ADC_IRQHandler(const_cast<ADC_HandleTypeDef*>(&mHandle));
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch the ADC conversion complete interrupt into a
 *          EndOfConversion callback.
 * \param   handle  The ADC handle from which the conversion complete ISR came.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* handle)
{
    ASSERT(handle);

    // Could only get here via interrupt: stop, as new requests will start anew
    if (HAL_ADC_Stop_IT(handle) == HAL_OK)
    {
        ASSERT(false);

        if (handle->Instance == ADC1) { CallbackEndOfConversion(adc1_callbacks, static_cast<uint16_t>(HAL_ADC_GetValue(handle))); }
        if (handle->Instance == ADC2) { CallbackEndOfConversion(adc2_callbacks, static_cast<uint16_t>(HAL_ADC_GetValue(handle))); }
        if (handle->Instance == ADC3) { CallbackEndOfConversion(adc3_callbacks, static_cast<uint16_t>(HAL_ADC_GetValue(handle))); }
    }
}

/**
 * \brief   ISR: route ADC1 interrupts to 'CallbackIRQ'.
 */
extern "C" void ADC1_IRQHandler(void)
{
    CallbackIRQ(adc1_callbacks);
}

/**
 * \brief   ISR: route ADC2 interrupts to 'CallbackIRQ'.
 */
extern "C" void ADC2_IRQHandler(void)
{
    CallbackIRQ(adc2_callbacks);
}

/**
 * \brief   ISR: route ADC3 interrupts to 'CallbackIRQ'.
 */
extern "C" void ADC3_IRQHandler(void)
{
    CallbackIRQ(adc3_callbacks);
}
