/**
 * \file    LIS3DSH.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Driver for the LIS3DSH accelerometer.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/components/LIS3DSH
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    10-2019
 */

#ifndef LIS3DSH_HPP_
#define LIS3DSH_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "drivers/Pin/Pin.hpp"
#include "drivers/SPI/SPI.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class LIS3DSH
{
public:
    /**
     * \enum    SampleFrequency
     * \brief   Sample frequency for accelerometer data.
     * \note    AN3393 - LIS3DSH - Application note - Table 2: Power consumption.
     */
    enum class SampleFrequency : uint8_t
    {
        _3_125_Hz,          // Low power
        _6_25_Hz,
        _12_5_Hz,
        _25_Hz,             // Normal
        _50_Hz,
        _100_Hz,            // High performance
        _400_Hz,
        _800_Hz,
        _1600_Hz
    };

    /**
     * \enum    Scale
     * \brief   Scale of the accelerometer data.
     * \note    AN3393 - LIS3DSH - Application note - Table 30: Control register 5 description.
     */
    enum class Scale : uint8_t
    {
        _2_G,               ///< Default
        _4_G,
        _6_G,
        _8_G,
        _16_G,
    };

    /**
     * \enum    AntiAliasingFilter
     * \brief   Anti-aliasing filter bandwidth.
     * \note    AN3393 - LIS3DSH - Application note - Table 30: Control register 5 description.
     */
    enum class AntiAliasingFilter : uint8_t
    {
        _800_Hz,            ///< Default
        _200_Hz,
        _400_Hz,
        _50_Hz
    };

    /**
     * \enum    FifoMode
     * \brief   FIFO mode selection.
     * \note    AN3393 - LIS3DSH - Application note - Table 37: FIFO_CTRL description.
     */
    enum class FifoMode : uint8_t
    {
        Bypass,
        Fifo,               ///< Default
        Stream,
        StreamThenFifo,
        BypassThenStream,
        BypassThenFifo
    };

    /**
     * \struct  Config
     * \brief   Configuration struct for LIS3DSH.
     */
    struct Config
    {
        /**
         * \brief   Constructor of the LIS3DSH configuration struct.
         * \param   sampleFrequency     Sample frequency for accelerometer data.
         * \param   scale               Scale of the accelerometer data. Default +/- 2G.
         * \param   antiAliasingFilter  Anti-aliasing filter bandwidth. Default 200 Hz.
         *
         */
        explicit Config(SampleFrequency sampleFrequency,
                        Scale scale = Scale::_2_G,
                        AntiAliasingFilter antiAliasingFilter = AntiAliasingFilter::_200_Hz) :
            mSampleFrequency(sampleFrequency),
            mScale(scale),
            mAntiAliasingFilter(antiAliasingFilter)
        { }

        SampleFrequency    mSampleFrequency;        ///< Sample frequency for accelerometer data.
        Scale              mScale;                  ///< Scale of the accelerometer data.
        AntiAliasingFilter mAntiAliasingFilter;     ///< Anti-aliasing filter bandwidth.
    };

    LIS3DSH(SPI& spi, PinIdPort chipSelect, PinIdPort motionInt1, PinIdPort motionInt2);
    virtual ~LIS3DSH();

    bool Init(const Config& config);
    bool IsInit() const;
    void Sleep();

    bool Enable();
    bool Disable();

    void SetHandler(const std::function<void(uint8_t length)>& handler);
    bool RetrieveAxesData(uint8_t* dest, uint8_t length);

private:
    SPI& mSpi;
    Pin  mChipSelect;
    Pin  mMotionInt1;
    Pin  mMotionInt2;
    bool mInitialized;
    uint8_t* mReadBuffer;
    uint8_t mODR;

    std::function<void(uint8_t length)> mHandler;

    bool SelfTest();
    bool Configure(const Config& config);
    bool PrepareReadBuffer(SampleFrequency sampleFrequency);
    bool ClearFifo();
    uint8_t GetSampleFrequencyAsODR(SampleFrequency sampleFrequency);
    uint8_t GetScaleAsFCALE(Scale scale);
    uint8_t GetAntiAliasingFilterAsBW(AntiAliasingFilter antiAliasingFilter);
    uint8_t GetFifoModeAsFMODE(FifoMode fifoMode);

    void ReadAxesCompleted();

    bool WriteRegister(uint8_t reg, const uint8_t* src, uint16_t length);
    bool ReadRegister(uint8_t reg, uint8_t* dest, uint16_t length);

    void CallbackInt1();
    void CallbackInt2();
};


#endif  // LIS3DSH_HPP_
