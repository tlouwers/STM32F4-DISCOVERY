/**
 * \file    LIS3DSH.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   LIS3DSH
 *
 * \brief   Driver for the LIS3DSH accelerometer.
 *
 * \details Intended is to use the hardware FIFO, which is read via SPI + DMA
 *          to minimize power consumption. If not using the hardware FIFO then
 *          the Data Ready signal is used, meaning a sample (X,Y,Z) is
 *          available at the configured sample frequency. Sample is read via
 *          SPI + DMA as well.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/Drivers/components/LIS3DSH
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.1
 * \date    05-2022
 */

#ifndef LIS3DSH_HPP_
#define LIS3DSH_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "interfaces/IInitable.hpp"
#include "interfaces/ISPI.hpp"
#include "drivers/Pin/Pin.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class LIS3DSH final : public IConfigInitable
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
     * \note    Used internally only: either no HW fifo is used (Bypass) or Stream is used.
     */
    enum class FifoMode : uint8_t
    {
        Bypass,
        Fifo,
        Stream,             ///< Default
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
         * \param   useHardwareFifo     Flag indicating hardware FIFO is to be used.
         * \param   sampleFrequency     Sample frequency for accelerometer data.
         * \param   scale               Scale of the accelerometer data. Default +/- 2G.
         * \param   antiAliasingFilter  Anti-aliasing filter bandwidth. Default 200 Hz.
         *
         */
        explicit Config(bool useHardwareFifo,
                        SampleFrequency sampleFrequency,
                        Scale scale = Scale::_2_G,
                        AntiAliasingFilter antiAliasingFilter = AntiAliasingFilter::_200_Hz) :
            mUseHardwareFifo(useHardwareFifo),
            mSampleFrequency(sampleFrequency),
            mScale(scale),
            mAntiAliasingFilter(antiAliasingFilter)
        { }

        bool               mUseHardwareFifo;        ///< Flag indicating hardware FIFO is to be used.
        SampleFrequency    mSampleFrequency;        ///< Sample frequency for accelerometer data.
        Scale              mScale;                  ///< Scale of the accelerometer data.
        AntiAliasingFilter mAntiAliasingFilter;     ///< Anti-aliasing filter bandwidth.
    };

    LIS3DSH(ISPI& spi, PinIdPort chipSelect, PinIdPort motionInt1, PinIdPort motionInt2);
    virtual ~LIS3DSH();

    bool Init(const IConfig& config) override;
    bool IsInit() const override;
    bool Sleep() override;

    bool Enable();
    bool Disable();

    void SetHandler(const std::function<void(uint8_t length)>& handler);
    bool RetrieveAxesData(uint8_t* dest, uint8_t length);

private:
    ISPI&    mSpi;
    Pin      mChipSelect;
    Pin      mMotionInt1;
    Pin      mMotionInt2;
    bool     mInitialized;
    uint8_t* mReadBuffer;
    uint8_t  mODR;
    bool     mUseHardwareFifo;

    std::function<void(uint8_t length)> mHandler;

    bool SelfTest();
    bool Configure(const IConfig& config);
    bool PrepareReadBuffer(uint8_t bufferSize);
    bool ClearFifo();
    uint8_t GetSampleFrequencyAsODR(SampleFrequency sampleFrequency);
    uint8_t GetScaleAsFSCALE(Scale scale);
    uint8_t GetAntiAliasingFilterAsBW(AntiAliasingFilter antiAliasingFilter);
    uint8_t GetFifoModeAsFMODE(FifoMode fifoMode);

    void ReadAxesCompleted();

    bool WriteRegister(uint8_t reg, const uint8_t* src, uint16_t length);
    bool ReadRegister(uint8_t reg, uint8_t* dest, uint16_t length);

    void CallbackInt1();
    void CallbackInt2();
};


#endif  // LIS3DSH_HPP_
