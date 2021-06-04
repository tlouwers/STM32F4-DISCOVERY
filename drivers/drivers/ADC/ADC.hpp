/**
 * \file    ADC.hpp
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
#include "interfaces/IADC.hpp"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    ADCInstance
 * \brief   Available ADC instances.
 */
enum class ADCInstance : uint8_t
{
    ADC_1 = 1,
    ADC_2 = 2,
    ADC_3 = 3
};


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \struct  ADCCallbacks
 * \brief   Data structure to contain callbacks for a SPI instance.
 */
struct ADCCallbacks {
    std::function<void()> callbackIRQ  = nullptr;                       ///< Callback to call when IRQ occurs.
    std::function<void(uint16_t)> callbackEndOfConversion = nullptr;    ///< Callback to call when End Of Conversion occurs.
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Adc final : public IADC, public IConfigInitable
{
public:
    /**
     * \enum    Channel
     * \brief   Available ADC channels.
     */
    enum class Channel: uint8_t
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
     * \brief   Available ADC resolutions.
     */
    enum class Resolution : uint8_t
    {
        _6_BIT,
        _8_BIT,
        _10_BIT,
        _12_BIT     ///< 2^12 = 4096 steps, default
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for ADC.
     */
    struct Config : public IConfig
    {
        /**
         * \brief   Constructor of the ADC configuration struct.
         * \param   interruptPriority   Priority of the interrupt.
         * \param   channel             The channel to capture data from.
         * \param   resolution          The resolution of the captured data.
         */
        Config(uint8_t interruptPriority, Channel channel, Resolution resolution = Resolution::_12_BIT) :
            mInterruptPriority(interruptPriority),
            mChannel(channel),
            mResolution(resolution)
        { }

        uint8_t    mInterruptPriority;  ///< Interrupt priority.
        Channel    mChannel;            ///< Channel to capture data from.
        Resolution mResolution;         ///< Resolution of the captured data.
    };


    explicit Adc(const ADCInstance& instance);
    virtual ~Adc();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool GetValue(uint16_t& value) override;
    bool GetValueInterrupt(const std::function<void(uint16_t)>& handler) override;

private:
    ADCInstance       mInstance;
    ADC_HandleTypeDef mHandle = {};
    ADCCallbacks&     mADCCallbacks;
    bool              mInitialized;

    void SetInstance(const ADCInstance& instance);
    void CheckAndEnableAHB2PeripheralClock(const ADCInstance& instance);
    void CheckAndDisableAHB2PeripheralClock(const ADCInstance& instance);
    uint32_t GetChannel(const Channel& channel);
    uint32_t GetResolution(const Resolution& resolution);
    void SetIRQn(IRQn_Type type, uint32_t preemptPrio, uint32_t subPrio);
    void CallbackIRQ() const;
};


#endif  // ADC_HPP_
