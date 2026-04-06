/**
 * \file    FakeLIS3DSH.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   FakeLIS3DSH
 *
 * \brief   FakeDriver for the LIS3DSH accelerometer.
 *
 * \details Need to call RetrieveAxesData manually, since no the accelerometer
 *          chip is simulated and does not generate a pin interrupt.
 *          Also fill in a size which is divisable by 6 for proper samples.
 *          When sawtooths are not used the data is returned as 0.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/components/LIS3DSH
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2021
 */

#ifndef FakeLIS3DSH_HPP_
#define FakeLIS3DSH_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <cstring>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/ISPI.hpp"
#include "drivers/Pin/Pin.hpp"
#if (SIMULATED_SENSOR_OUTPUT_DATA == SAWTOOTH_SIGNAL)
#   include "utility/Sawtooth/Sawtooth.hpp"
#endif


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class FakeLIS3DSH final : public IConfigInitable
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
    struct Config : public IConfig
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

    FakeLIS3DSH(ISPI& spi, PinIdPort chipSelect, PinIdPort motionInt1, PinIdPort motionInt2) :
        mInitialized(false)
#if (SIMULATED_SENSOR_OUTPUT_DATA == SAWTOOTH_SIGNAL)
        , mXaxisSawTooth(100,  8)  // 0..99 in  8 steps of 12
        , mYaxisSawTooth( 90,  6)  // 0..89 in  6 steps of 15
        , mZaxisSawTooth( 80, 10)  // 0..79 in 10 steps of  8
#endif
    {
        UNUSED(spi);
        UNUSED(chipSelect);
        UNUSED(motionInt1);
        UNUSED(motionInt2);
    }
    virtual ~FakeLIS3DSH() {}

    bool Init(const IConfig& config) override { UNUSED(config); mInitialized = true; return true; }
    bool IsInit() const override { return mInitialized; }
    bool Sleep() override { mInitialized = false; return true; }

    bool Enable() { std::memset(mMotionArray, 0, sizeof(mMotionArray)); return mInitialized; }
    bool Disable() { return mInitialized; }

    void SetHandler(const std::function<void(uint8_t length)>& handler) { UNUSED(handler); }
    bool RetrieveAxesData(uint8_t* dest, uint8_t length)
    {
        if (dest == nullptr) { return false; }
        if (length == 0)     { return false; }
        if (length % 6 != 0) { return false; }

#if (SIMULATED_SENSOR_OUTPUT_DATA == SAWTOOTH_SIGNAL)
        uint32_t i = 0;
        while (i < (length / 6))
        {
            uint16_t X = mXaxisSawTooth.Next();
            uint16_t Y = mYaxisSawTooth.Next();
            uint16_t Z = mZaxisSawTooth.Next();

            mMotionArray[i++] = static_cast<uint8_t>((X & 0xFF00) >> 8);
            mMotionArray[i++] = static_cast<uint8_t>((X & 0x00FF)     );
            mMotionArray[i++] = static_cast<uint8_t>((Y & 0xFF00) >> 8);
            mMotionArray[i++] = static_cast<uint8_t>((Y & 0x00FF)     );
            mMotionArray[i++] = static_cast<uint8_t>((Z & 0xFF00) >> 8);
            mMotionArray[i++] = static_cast<uint8_t>((Z & 0x00FF)     );
        }
#endif
        std::memcpy(dest, mMotionArray, length);

        return true;
    }

private:
    bool mInitialized;
    uint8_t mMotionArray[150] = {};

    std::function<void(uint8_t length)> mHandler;

#if (SIMULATED_SENSOR_OUTPUT_DATA == SAWTOOTH_SIGNAL)
    Sawtooth mXaxisSawTooth;
    Sawtooth mYaxisSawTooth;
    Sawtooth mZaxisSawTooth;
#endif
};


#endif  // FakeLIS3DSH_HPP_
