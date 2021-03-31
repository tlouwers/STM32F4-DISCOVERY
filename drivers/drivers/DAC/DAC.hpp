/**
 * \file    DAC.hpp
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
 * \note    Either use software trigger, or use DMA in combination with a timer - assume BasicTimer 6 or 7 is used?
 *          Can be any timer. Also can be external trigger.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    03-2021
 */

#ifndef DAC_HPP_
#define DAC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   DAC peripheral driver class.
 */
class Dac
{
public:
    /**
     * \enum    Channel
     * \brief   Available DAC channels.
     */
    enum class Channel : uint8_t
    {
        CHANNEL_1,
        CHANNEL_2
    };

    /**
     * \enum    Precision
     * \brief   Available DAC precision (in bits).
     * \details Expressed as: Vout = (Vref x D) / 2^N
     */
    enum class Precision : uint8_t
    {
        _8_BIT_R,   ///< 2^8  =  256 steps (right aligned)
        _12_BIT_L,  ///< 2^12 = 4096 steps (left aligned)
        _12_BIT_R   ///< 2^12 = 4096 steps (right aligned), default
    };

    /**
     * \enum    Trigger
     * \brief   Available DAC triggers.
     * \note    Timer6 and Timer7 are dedicated for DAC if not using Software trigger.
     */
    enum class Trigger : uint8_t
    {
        NONE,       ///< Default
        TIMER_2,
        TIMER_4,
        TIMER_5,
        TIMER_6,
        TIMER_7,
        TIMER_8,
        EXT_LINE_9,
        //SOFTWARE      // Requires HAL_DAC_Start() after loading value in output register
    };

    /**
     * \struct  ChannelConfig
     * \brief   Configuration struct for DAC channel.
     */
    struct ChannelConfig
    {
        /**
         * \brief   Constructor for the DAC channel configuration struct.
         * \param   precision   The precision or alignment of the data.
         * \param   trigger     The trigger for the channel.
         */
        ChannelConfig(Precision precision = Precision::_12_BIT_R, Trigger trigger = Trigger::NONE) :
            mStarted(false),
            mPrecision(precision),
            mTrigger(trigger)
        { }

        bool      mStarted;     ///< Flag, indicated the channel is started.
        Precision mPrecision;   ///< Precision or alignment of the data.
        Trigger   mTrigger;     ///< Trigger for the channel.
    };

    /**
     * \struct  Waveform
     * \brief   Description to hold parameters of a waveform (buffer).
     */
    struct Waveform
    {
        /**
         * \brief   Constructor of the Waveform struct.
         */
        Waveform() :
            mValues(nullptr),
            mLength(0),
            mIndex(0)
        { }

        uint16_t* mValues;  ///< Pointer to the buffer containing the waveform values.
        uint16_t  mLength;  ///< Length of the buffer.
        uint16_t  mIndex;   ///< Index of the buffer pointing to current value.
    };


    Dac();
    virtual ~Dac();

    bool Init();
    bool IsInit() const;
    void Sleep();

    const DAC_HandleTypeDef* GetPeripheralHandle() const;
    DMA_HandleTypeDef*& GetDmaChannel1Handle();
    DMA_HandleTypeDef*& GetDmaChannel2Handle();

    bool ConfigureChannel(const Channel& channel, const ChannelConfig& channelConfig);
    bool ConfigureWaveform(const Channel& channel, uint16_t* values, uint16_t length);

    bool SetValue(const Channel& channel, uint16_t value);

    bool StartWaveform(const Channel& channel);
    bool StopWaveform(const Channel& channel);

private:
    DAC_HandleTypeDef mHandle = {};
    bool              mInitialized;
    ChannelConfig     mChannel1 = {};
    ChannelConfig     mChannel2 = {};
    Waveform          mWaveformChannel1 = {};
    Waveform          mWaveformChannel2 = {};

    void CheckAndEnableAHB1PeripheralClock();

    uint32_t GetTrigger(const Trigger& trigger);
    uint32_t GetAlignment(const Precision& precision);

    bool StartChannel(const Channel& channel);
    bool StopChannel(const Channel& channel);
    void SetWaveform(const Channel& channel, uint16_t* values, uint16_t length);
};


#endif  // DAC_HPP_
