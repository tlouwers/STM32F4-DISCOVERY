/**
 * \file    Adc.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Adc peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/Adc
 *
 * \note    Using right alignment only to be consistent with all resolutions.
 *          GetValue uses: Adc(input) = value * (Vref / (Adc(resolution) + 1) ) - for 12-bit: Adc(input) = value * (3.3V / (0xFFF + 1) --> var = (value * (0xFFF + 1)) / 3.3V
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "drivers/Adc/Adc.hpp"
#include "utility/Assert/Assert.h"
#include "utility/ScopedIrqMask/ScopedIrqMask.hpp"
#include "stm32f4xx_hal_adc.h"


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static AdcCallbacks adc1_callbacks {};
static AdcCallbacks adc2_callbacks {};
static AdcCallbacks adc3_callbacks {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Call the callbackIRQ, if configured.
 * \param   adc_callbacks   Structure containing the callbackIRQ to call.
 */
static void CallbackIRQ(const AdcCallbacks& adc_callbacks)
{
    if (adc_callbacks.mCallbackIRQ)
    {
        adc_callbacks.mCallbackIRQ();
    }
}

/**
 * \brief   Call the callbackEndOfConversion, if configured.
 * \param   adc_callbacks   Structure containing the callbackEndOfConversion to call.
 */
static void CallbackEndOfConversion(const AdcCallbacks& adc_callbacks, uint16_t value)
{
    if (adc_callbacks.mCallbackEndOfConversion)
    {
        adc_callbacks.mCallbackEndOfConversion(value);
    }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, prepares the internal Adc administration.
 */
Adc::Adc(const AdcInstance& instance) :
    mInstance(instance),
    mAdcCallbacks( (instance == AdcInstance::ADC_1) ? (adc1_callbacks) : ( (instance == AdcInstance::ADC_2) ? (adc2_callbacks) : (adc3_callbacks) ) ),
    mInitialized(false)
{
    SetInstance(instance);
}

/**
 * \brief   Destructor, stops Adc channels.
 * \note    DisconnectCallbacks is also invoked unconditionally here as a
 *          safety net: ADC1/2/3 share ADC_IRQn, so a stale lambda holding
 *          this object's `this` would otherwise be dispatched by a peer
 *          instance after destruction.
 */
Adc::~Adc()
{
    Sleep();
    DisconnectCallbacks();
}

/**
 * \brief   Initializes the Adc instance with the given configuration.
 * \param   config  The configuration for the Adc instance to use.
 * \returns True if the configuration could be applied, else false.
 * \note    ADCCLK = PCLK2 / Config::mPrescaler. The F407 datasheet caps
 *          ADCCLK at 36 MHz; Init() rejects (returns false) a prescaler that
 *          would exceed that for the active Board clock profile (DIV2 is fine
 *          for the HSE-direct 8 MHz default; the 168 MHz PLL path gives
 *          PCLK2 = 84 MHz so DIV4 or higher is required there).
 */
bool Adc::Init(const IConfig& config)
{
    CheckAndEnableAPB2PeripheralClock(mInstance);

    EXPECT(config.ConfigId() == Config::Id());
    if (config.ConfigId() != Config::Id()) { return false; }

    const Config& cfg = static_cast<const Config&>(config);

    // Enforce the ADCCLK ceiling for the active board clock profile: ADCCLK =
    // PCLK2 / prescaler must stay <= 36 MHz (F407 DS8626 §6.3.15 / RM0090 §13.5).
    // Overclocking the ADC silently degrades conversion accuracy, so reject a
    // prescaler that the current PCLK2 cannot satisfy rather than trust the caller.
    static const uint32_t ADCCLK_MAX_HZ = 36000000U;
    const uint32_t adcClk = HAL_RCC_GetPCLK2Freq() / GetPrescalerDivider(cfg.mPrescaler);
    EXPECT(adcClk <= ADCCLK_MAX_HZ);
    if (adcClk > ADCCLK_MAX_HZ) { return false; }

    mHandle.Init.ClockPrescaler        = GetPrescaler(cfg.mPrescaler);
    mHandle.Init.Resolution            = GetResolution(cfg.mResolution);
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
        SetIRQn(ADC_IRQn, cfg.mInterruptPriority, 0);

        // Configure channel
        ADC_ChannelConfTypeDef adcChannelConfig = {};
        adcChannelConfig.Channel      = GetChannel(cfg.mChannel);
        adcChannelConfig.Offset       = 0;
        adcChannelConfig.Rank         = 1;
        adcChannelConfig.SamplingTime = GetSamplingTime(cfg.mSamplingTime);

        if (HAL_ADC_ConfigChannel(&mHandle, &adcChannelConfig) == HAL_OK)
        {
            mAdcCallbacks.mCallbackIRQ = [this]() { this->CallbackIRQ(); };
            mInitialized = true;
            return true;
        }
    }
    return false;
}

/**
 * \brief   Indicate if Adc is initialized.
 * \returns True if Adc is initialized, else false.
 */
bool Adc::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the Adc module in sleep mode.
 * \details Aborts ongoing captures.
 * \returns True if Adc module could be put in sleep mode, else false.
 * \note    ADC1/2/3 share ADC_IRQn, so the NVIC line is only disabled when
 *          no other Adc instance still holds an active callback slot.
 */
bool Adc::Sleep()
{
    // Stop is best-effort: a not-running ADC returns an error here, which must
    // not block the DeInit teardown. Surface a genuine stop failure only when
    // DeInit also fails (below).
    HAL_ADC_Stop(&mHandle);

    if (HAL_ADC_DeInit(&mHandle) != HAL_OK) { return false; }

    mInitialized = false;

    DisconnectCallbacks();

    if ((nullptr == adc1_callbacks.mCallbackIRQ) &&
        (nullptr == adc2_callbacks.mCallbackIRQ) &&
        (nullptr == adc3_callbacks.mCallbackIRQ))
    {
        HAL_NVIC_DisableIRQ(ADC_IRQn);
    }

    CheckAndDisableAPB2PeripheralClock(mInstance);
    return true;
}

/**
 * \brief   Sample a value with the Adc from input.
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
 * \brief   Sample a value with the Adc from input using interrupts.
 * \param   handler     Callback to call when sampling completed.
 * \returns True if the sampling could be started, else false.
 */
bool Adc::GetValueInterrupt(const std::function<void(uint16_t)>& handler)
{
    if (!mInitialized) { return false; }

    // ADC1/2/3 share ADC_IRQn. Mask the shared line, start the conversion, and
    // arm the callback slot only on HAL_OK: this closes the TOCTOU window (an ISR
    // reading a half-assigned std::function) and avoids leaving a stale handler
    // armed if HAL_ADC_Start_IT rejects the request.
    ScopedIrqMask mask(ADC_IRQn);

    if (HAL_ADC_Start_IT(&mHandle) != HAL_OK) { return false; }

    mAdcCallbacks.mCallbackEndOfConversion = handler;
    return true;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Set the Adc instance into internal administration.
 * \param   instance    The Adc instance to use.
 * \note    Asserts if the Adc instance is invalid.
 */
void Adc::SetInstance(const AdcInstance& instance)
{
    switch (instance)
    {
        case AdcInstance::ADC_1: mHandle.Instance = ADC1; break;
        case AdcInstance::ADC_2: mHandle.Instance = ADC2; break;
        case AdcInstance::ADC_3: mHandle.Instance = ADC3; break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Enable the APB2 peripheral clock for the given Adc instance.
 * \param   instance    The Adc instance to enable the clock for.
 * \note    Asserts if not a valid Adc instance provided.
 */
void Adc::CheckAndEnableAPB2PeripheralClock(const AdcInstance& instance)
{
    switch (instance)
    {
        case AdcInstance::ADC_1: __HAL_RCC_ADC1_CLK_ENABLE(); break;
        case AdcInstance::ADC_2: __HAL_RCC_ADC2_CLK_ENABLE(); break;
        case AdcInstance::ADC_3: __HAL_RCC_ADC3_CLK_ENABLE(); break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }
}

/**
 * \brief   Disable the APB2 peripheral clock for the given Adc instance.
 * \param   instance    The Adc instance to disable the clock for.
 * \note    Asserts if not a valid Adc instance provided.
 */
void Adc::CheckAndDisableAPB2PeripheralClock(const AdcInstance& instance)
{
    switch (instance)
    {
        case AdcInstance::ADC_1: __HAL_RCC_ADC1_CLK_DISABLE(); break;
        case AdcInstance::ADC_2: __HAL_RCC_ADC2_CLK_DISABLE(); break;
        case AdcInstance::ADC_3: __HAL_RCC_ADC3_CLK_DISABLE(); break;
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
 * \brief   Get the translated ADCCLK prescaler value.
 * \param   prescaler   The desired prescaler.
 * \returns Translated prescaler value.
 */
uint32_t Adc::GetPrescaler(const Prescaler& prescaler)
{
    uint32_t prescaler_value = ADC_CLOCK_SYNC_PCLK_DIV2;

    switch (prescaler)
    {
        case Prescaler::DIV2: { prescaler_value = ADC_CLOCK_SYNC_PCLK_DIV2; } break;
        case Prescaler::DIV4: { prescaler_value = ADC_CLOCK_SYNC_PCLK_DIV4; } break;
        case Prescaler::DIV6: { prescaler_value = ADC_CLOCK_SYNC_PCLK_DIV6; } break;
        case Prescaler::DIV8: { prescaler_value = ADC_CLOCK_SYNC_PCLK_DIV8; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return prescaler_value;
}

/**
 * \brief   Get the integer division factor for the given prescaler.
 * \param   prescaler   The desired prescaler.
 * \returns The numeric divider (2, 4, 6 or 8) used to derive ADCCLK from PCLK2.
 * \note    Used by Init() to verify ADCCLK stays within the F407 36 MHz limit.
 */
uint32_t Adc::GetPrescalerDivider(const Prescaler& prescaler)
{
    uint32_t divider = 2;

    switch (prescaler)
    {
        case Prescaler::DIV2: { divider = 2; } break;
        case Prescaler::DIV4: { divider = 4; } break;
        case Prescaler::DIV6: { divider = 6; } break;
        case Prescaler::DIV8: { divider = 8; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return divider;
}

/**
 * \brief   Get the translated sample-and-hold time value.
 * \param   samplingTime    The desired sampling time.
 * \returns Translated sampling time value.
 */
uint32_t Adc::GetSamplingTime(const SamplingTime& samplingTime)
{
    uint32_t sampling_value = ADC_SAMPLETIME_15CYCLES;

    switch (samplingTime)
    {
        case SamplingTime::_3_CYCLES:   { sampling_value = ADC_SAMPLETIME_3CYCLES;   } break;
        case SamplingTime::_15_CYCLES:  { sampling_value = ADC_SAMPLETIME_15CYCLES;  } break;
        case SamplingTime::_28_CYCLES:  { sampling_value = ADC_SAMPLETIME_28CYCLES;  } break;
        case SamplingTime::_56_CYCLES:  { sampling_value = ADC_SAMPLETIME_56CYCLES;  } break;
        case SamplingTime::_84_CYCLES:  { sampling_value = ADC_SAMPLETIME_84CYCLES;  } break;
        case SamplingTime::_112_CYCLES: { sampling_value = ADC_SAMPLETIME_112CYCLES; } break;
        case SamplingTime::_144_CYCLES: { sampling_value = ADC_SAMPLETIME_144CYCLES; } break;
        case SamplingTime::_480_CYCLES: { sampling_value = ADC_SAMPLETIME_480CYCLES; } break;
        default: ASSERT(false); while(1) { __NOP(); } break;    // Impossible selection
    }

    return sampling_value;
}

/**
 * \brief   Lower level configuration for the Adc interrupts.
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
 * \brief   Generic Adc IRQ callback. Will propagate other interrupts.
 */
void Adc::CallbackIRQ()
{
    HAL_ADC_IRQHandler(&mHandle);
}

/**
 * \brief   Drop both callback std::functions for this Adc instance.
 * \note    Called from Sleep() and as a destructor safety net so that no
 *          stale lambda capturing `this` outlives the object on the
 *          shared ADC_IRQn line.
 */
void Adc::DisconnectCallbacks()
{
    mAdcCallbacks.mCallbackIRQ             = nullptr;
    mAdcCallbacks.mCallbackEndOfConversion = nullptr;
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch the Adc conversion complete interrupt into
 *          an EndOfConversion callback.
 * \param   handle  The Adc handle from which the conversion complete ISR came.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* handle)
{
    ASSERT(handle);

    // Could only get here via interrupt: stop, as new requests will start anew
    if (HAL_ADC_Stop_IT(handle) != HAL_OK) { ASSERT(false); }

    if      (handle->Instance == ADC1) { CallbackEndOfConversion(adc1_callbacks, static_cast<uint16_t>(HAL_ADC_GetValue(handle))); }
    else if (handle->Instance == ADC2) { CallbackEndOfConversion(adc2_callbacks, static_cast<uint16_t>(HAL_ADC_GetValue(handle))); }
    else if (handle->Instance == ADC3) { CallbackEndOfConversion(adc3_callbacks, static_cast<uint16_t>(HAL_ADC_GetValue(handle))); }
}

/**
 * \brief   ISR: route ADC1, ADC2, ADC3 interrupts to 'CallbackIRQ'.
 */
extern "C" void ADC_IRQHandler(void)
{
    CallbackIRQ(adc1_callbacks);
    CallbackIRQ(adc2_callbacks);
    CallbackIRQ(adc3_callbacks);
}
