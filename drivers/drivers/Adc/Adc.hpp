/**
 * \file    Adc.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Adc
 *
 * \brief   Adc peripheral driver class.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/drivers/Adc
 *
 * \note    Only single conversion, only software trigger. Either with blocking
 *          (polling) method or using interrupt.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2021
 */

#ifndef ADC_HPP_
#define ADC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/IAdc.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    AdcInstance
 * \brief   Available Adc instances.
 */
enum class AdcInstance : uint8_t
{
    ADC_1 = 1,
    ADC_2 = 2,
    ADC_3 = 3
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  AdcCallbacks
 * \brief   Data structure to contain callbacks for an Adc instance.
 */
struct AdcCallbacks {
    std::function<void()> mCallbackIRQ  = nullptr;                       ///< Callback to call when IRQ occurs.
    std::function<void(uint16_t)> mCallbackEndOfConversion = nullptr;    ///< Callback to call when End Of Conversion occurs.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Adc final : public IAdc, public IConfigInitable
{
public:
    /**
     * \enum    Channel
     * \brief   Available Adc channels.
     */
    enum class Channel : uint8_t
    {
        CHANNEL_0,
        CHANNEL_1,
        CHANNEL_2,
        CHANNEL_3,
        CHANNEL_4,
        CHANNEL_5,
        CHANNEL_6,
        CHANNEL_7,
        CHANNEL_8,
        CHANNEL_9,
        CHANNEL_10,
        CHANNEL_11,
        CHANNEL_12,
        CHANNEL_13,
        CHANNEL_14,
        CHANNEL_15
    };

    /**
     * \enum    Resolution
     * \brief   Available Adc resolutions.
     */
    enum class Resolution : uint8_t
    {
        _6_BIT,
        _8_BIT,
        _10_BIT,
        _12_BIT     ///< 2^12 = 4096 steps, default
    };

    /**
     * \enum    Prescaler
     * \brief   ADCCLK prescaler off PCLK2. The F407 datasheet caps ADCCLK
     *          at 36 MHz -- pick a divider so PCLK2 / N stays at or below
     *          that for the active Board clock profile.
     */
    enum class Prescaler : uint8_t
    {
        DIV2,       ///< PCLK2 / 2, default (safe for PCLK2 <= 72 MHz)
        DIV4,       ///< PCLK2 / 4
        DIV6,       ///< PCLK2 / 6
        DIV8        ///< PCLK2 / 8
    };

    /**
     * \enum    SamplingTime
     * \brief   Sample-and-hold time in ADCCLK cycles. RM0090 §13.4: pick
     *          from the source impedance -- low cycle counts only suit
     *          low-impedance sources.
     */
    enum class SamplingTime : uint8_t
    {
        _3_CYCLES,
        _15_CYCLES,     ///< Default
        _28_CYCLES,
        _56_CYCLES,
        _84_CYCLES,
        _112_CYCLES,
        _144_CYCLES,
        _480_CYCLES
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for Adc.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the Adc configuration struct.
         * \param   interruptPriority   Priority of the interrupt.
         * \param   channel             The channel to capture data from.
         * \param   resolution          The resolution of the captured data.
         * \param   prescaler           ADCCLK prescaler off PCLK2 -- default
         *                              DIV2, preserving the previous fixed value.
         * \param   samplingTime        Sample-and-hold time -- default 15
         *                              cycles, preserving the previous fixed value.
         */
        Config(uint8_t interruptPriority, Channel channel,
               Resolution resolution = Resolution::_12_BIT,
               Prescaler prescaler = Prescaler::DIV2,
               SamplingTime samplingTime = SamplingTime::_15_CYCLES) :
            mInterruptPriority(interruptPriority),
            mChannel(channel),
            mResolution(resolution),
            mPrescaler(prescaler),
            mSamplingTime(samplingTime)
        { }

        uint8_t      mInterruptPriority;  ///< Interrupt priority.
        Channel      mChannel;            ///< Channel to capture data from.
        Resolution   mResolution;         ///< Resolution of the captured data.
        Prescaler    mPrescaler;          ///< ADCCLK prescaler off PCLK2.
        SamplingTime mSamplingTime;       ///< Sample-and-hold time in cycles.

        /**
         * \brief   Unique runtime type tag for this Config.
         * \returns Address stable and unique to this Config type.
         */
        static const void* Id() { static const char sTag = 0; return &sTag; }

        /**
         * \brief   Runtime type identity, see IConfig::ConfigId().
         * \returns Address stable and unique to this Config type.
         */
        const void* ConfigId() const override { return Id(); }
    };


    explicit Adc(const AdcInstance& instance);
    virtual ~Adc();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool GetValue(uint16_t& value) override;
    bool GetValueInterrupt(const std::function<void(uint16_t)>& handler) override;

    // Explicit disabled constructors/operators
    Adc(const Adc&)            = delete;
    Adc& operator=(const Adc&) = delete;
    Adc(Adc&&)                 = delete;
    Adc& operator=(Adc&&)      = delete;

private:
    AdcInstance       mInstance;
    ADC_HandleTypeDef mHandle = {};
    AdcCallbacks&     mAdcCallbacks;
    bool              mInitialized;

    void SetInstance(const AdcInstance& instance);
    void CheckAndEnableAPB2PeripheralClock(const AdcInstance& instance);
    void CheckAndDisableAPB2PeripheralClock(const AdcInstance& instance);
    uint32_t GetChannel(const Channel& channel);
    uint32_t GetResolution(const Resolution& resolution);
    uint32_t GetPrescaler(const Prescaler& prescaler);
    uint32_t GetPrescalerDivider(const Prescaler& prescaler);
    uint32_t GetSamplingTime(const SamplingTime& samplingTime);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackIRQ();
    void DisconnectCallbacks();
};


#endif  // ADC_HPP_
