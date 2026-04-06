/**
 * \file    LIS3DSH.cpp
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

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "components/LIS3DSH/LIS3DSH.hpp"
#include "utility/Assert/Assert.h"
#include <algorithm>
#include <cstring>


/************************************************************************/
/* Register Map                                                         */
/************************************************************************/
/*
static constexpr uint8_t OUT_T      = 0x0C;
static constexpr uint8_t INFO1      = 0x0D;
static constexpr uint8_t INFO2      = 0x0E;
*/
static constexpr uint8_t WHO_AM_I   = 0x0F;
/*
static constexpr uint8_t OFF_X      = 0x10;
static constexpr uint8_t OFF_Y      = 0x11;
static constexpr uint8_t OFF_Z      = 0x12;
static constexpr uint8_t CS_X       = 0x13;
static constexpr uint8_t CS_Y       = 0x14;
static constexpr uint8_t CS_Z       = 0x15;
static constexpr uint8_t LC_L       = 0x16;
static constexpr uint8_t LC_H       = 0x17;
static constexpr uint8_t STAT       = 0x18;
static constexpr uint8_t PEAK1      = 0x19;
static constexpr uint8_t PEAK2      = 0x1A;
static constexpr uint8_t VFC_1      = 0x1B;
static constexpr uint8_t VFC_2      = 0x1C;
static constexpr uint8_t VFC_3      = 0x1D;
static constexpr uint8_t VFC_4      = 0x1E;
static constexpr uint8_t THRS3      = 0x1F;
*/
static constexpr uint8_t CTRL_REG4  = 0x20;
static constexpr uint8_t CTRL_REG1  = 0x21;
static constexpr uint8_t CTRL_REG2  = 0x22;
static constexpr uint8_t CTRL_REG3  = 0x23;
static constexpr uint8_t CTRL_REG5  = 0x24;
static constexpr uint8_t CTRL_REG6  = 0x25;
static constexpr uint8_t STATUS     = 0x27;
static constexpr uint8_t OUT_X_L    = 0x28;
static constexpr uint8_t OUT_X_H    = 0x29;
static constexpr uint8_t OUT_Y_L    = 0x2A;
static constexpr uint8_t OUT_Y_H    = 0x2B;
static constexpr uint8_t OUT_Z_L    = 0x2C;
static constexpr uint8_t OUT_Z_H    = 0x2D;
static constexpr uint8_t FIFO_CTRL  = 0x2E;
static constexpr uint8_t FIFO_SRC   = 0x2F;
/*
// Omitted: ST1_X: SM1 code register (X=1-16)
static constexpr uint8_t TIM4_1     = 0x50;
static constexpr uint8_t TIM3_1     = 0x51;
static constexpr uint8_t TIM2_1     = 0x52;
static constexpr uint8_t TIM1_1     = 0x54;
static constexpr uint8_t THRS2_1    = 0x56;
static constexpr uint8_t THRS1_1    = 0x57;
static constexpr uint8_t MASK1_B    = 0x59;
static constexpr uint8_t MASK1_A    = 0x5A;
static constexpr uint8_t SETT1      = 0x5B;
static constexpr uint8_t PR1        = 0x5C;
static constexpr uint8_t TC1        = 0x5D;
static constexpr uint8_t OUTS1      = 0x5F;
// Omitted: ST2_X: SM2 code register (X=1-16)
static constexpr uint8_t TIM4_2      = 0x70;
static constexpr uint8_t TIM3_2      = 0x71;
static constexpr uint8_t TIM2_2      = 0x72;
static constexpr uint8_t TIM1_2      = 0x74;
static constexpr uint8_t THRS2_2     = 0x76;
static constexpr uint8_t THRS1_2     = 0x77;
static constexpr uint8_t DES2        = 0x78;
static constexpr uint8_t MASK2_B     = 0x79;
static constexpr uint8_t MASK2_A     = 0x7A;
static constexpr uint8_t SETT2       = 0x7B;
static constexpr uint8_t PR2         = 0x7C;
static constexpr uint8_t TC2         = 0x7D;
static constexpr uint8_t OUTS2       = 0x7F;
*/


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
static constexpr uint8_t IDENTIFIER       = 0x3F;
static constexpr uint8_t READ_MASK        = 0x80;
static constexpr uint8_t WATERMARK_LEVEL  = 0x19;                       // 25 samples X,Y,Z default - fifo size max = 32
static constexpr uint8_t READ_BUFFER_SIZE = 3 * 2 * WATERMARK_LEVEL;    // X,Y,Z * int16_t * 25 samples (in fifo)
static constexpr uint8_t BDU              = 0;                          // 0: disabled (default if fifo is used), 1: enabled
static constexpr uint8_t AXES_ENABLED     = 0x07;
static constexpr uint8_t AXES_DISABLED    = 0x00;
static constexpr uint8_t FIFO_EMPTY       = 0x20;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, configures ChipSelect, INT1 and INT2 pins.
 * \param   spi         SPI peripheral driver class.
 * \param   chipSelect  Pin ChipSelect, needed for SPI communication, toggled
 *                      within this class.
 * \param   motionInt1  Pin INT1, toggled by LIS3DSH.
 * \param   motionInt2  Pin INT2, toggled by LIS3DSH.
 */
LIS3DSH::LIS3DSH(ISPI& spi, PinIdPort chipSelect, PinIdPort motionInt1, PinIdPort motionInt2) :
    mSpi(spi),
    mChipSelect(chipSelect, Level::HIGH),
    mMotionInt1(motionInt1, PullUpDown::HIGHZ),
    mMotionInt2(motionInt2, PullUpDown::HIGHZ),
    mInitialized(false),
    mReadBuffer(nullptr),
    mODR(0)
{
    mMotionInt1.Interrupt(Trigger::RISING, [this]() { this-> CallbackInt1(); }, false );
    mMotionInt2.Interrupt(Trigger::RISING, [this]() { this-> CallbackInt2(); }, false );
}

/**
 * \brief   Destructor, configures pins to HIGHZ, deletes read buffer.
 */
LIS3DSH::~LIS3DSH()
{
    Sleep();
}

/**
 * \brief   Initializes the LIS3DSH, performs test to check if it can
 *          communicate with the sensor, then configures the sensor for fifo
 *          based readouts. Fifo is cleared before use.
 * \param   config  Configuration struct for LIS3DSH.
 * \returns True if the sensor could be initialized, else false.
 */
bool LIS3DSH::Init(const IConfig& config)
{
    mChipSelect.Configure(Level::HIGH);

    bool result = SelfTest();

    if (result)
    {
        result &= Configure(config);
        EXPECT(result);

        if (result)
        {
            result &= ClearFifo();
            EXPECT(result);

            mInitialized = true;
        }
    }

    return result;
}

/**
 * \brief   Indicate if LIS3DSH is initialized.
 * \returns True if LIS3DSH is initialized, else false.
 */
bool LIS3DSH::IsInit() const
{
    return mInitialized;
}

/**
 * \brief   Puts the LIS3DSH module in sleep mode.
 * \details Configures pins to HIGHZ, deletes read buffer.
 * \returns True if the LIS3DSH module could be put in sleep mode, else false.
 */
bool LIS3DSH::Sleep()
{
    bool result = Disable();
    ASSERT(result);

    result &= mMotionInt1.InterruptRemove();
    ASSERT(result);
    result &= mMotionInt2.InterruptRemove();
    ASSERT(result);

    mChipSelect.Configure(PullUpDown::HIGHZ);
    mMotionInt1.Configure(PullUpDown::HIGHZ);
    mMotionInt2.Configure(PullUpDown::HIGHZ);

    mInitialized = false;

    if (mReadBuffer)
    {
        delete[] mReadBuffer;
        mReadBuffer = nullptr;
    }

    return result;
}

/**
 * \brief   Start data acquisition.
 * \returns True if acquisition could be started successfully, else false.
 * \note    AN3393 - LIS3DSH - Application note - 10.3.1 Bypass mode - last few lines.
 */
bool LIS3DSH::Enable()
{
    if (mInitialized)
    {
        uint8_t val = 0;
        if ( ReadRegister(FIFO_CTRL, &val, 1) )
        {
            val = val & 0x1F;       // Clear mode: sets it to 'bypass'
            uint8_t FMODE = GetFifoModeAsFMODE(FifoMode::Stream);
            val = (val | FMODE);    // Now apply a mode again to start acquisition

            mMotionInt1.InterruptEnable();
            mMotionInt2.InterruptEnable();

            return WriteRegister(FIFO_CTRL, &val, 1);
        }
    }
    return false;
}

/**
 * \brief   Stop data acquisition.
 * \returns True if acquisition could be stopped successfully, else false.
 * \note    AN3393 - LIS3DSH - Application note - 10.3.1 Bypass mode - last few lines.
 */
bool LIS3DSH::Disable()
{
    if (mInitialized)
    {
        uint8_t val = 0;
        if ( ReadRegister(FIFO_CTRL, &val, 1) )
        {
            val = val & 0x1F;       // Clear mode: sets it to 'bypass'

            mMotionInt1.InterruptDisable();
            mMotionInt2.InterruptDisable();

            return WriteRegister(FIFO_CTRL, &val, 1);
        }
    }
    return false;
}

/**
 * \brief   Set handler to call when data is available.
 * \param   handler  Handler to call when data is available.
 */
void LIS3DSH::SetHandler(const std::function<void(uint8_t length)>& handler)
{
    mHandler = handler;
}

/**
 * \brief   Retrieve axes data retrieved from LIS3DSH fifo.
 * \param   dest    The buffer to copy the retrieved axes data into.
 * \param   length  The number of bytes to copy. This is intended to be the
 *                  number acquired when the data available handler is triggered.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool LIS3DSH::RetrieveAxesData(uint8_t* dest, uint8_t length)
{
    EXPECT(dest);
    EXPECT(length > 0);
    EXPECT(length <= READ_BUFFER_SIZE);

    if (dest == nullptr) { return false; }
    if (length == 0)     { return false; }
    if (length > READ_BUFFER_SIZE) { return false; }

    std::memcpy(dest, mReadBuffer, length);

    return true;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Read the WHO_AM_I register a number of times to check if there
 *          is communication possible with the LIS3DSH.
 * \returns True if the register could be read successfully, else false.
 */
bool LIS3DSH::SelfTest()
{
    bool result = false;

    for (auto i = 0; i < 25; i++)
    {
        uint8_t dest = 0;
        if (ReadRegister(WHO_AM_I, &dest, 1))
        {
            if (dest == IDENTIFIER)
            {
                result = true;
                break;
            }
            else
            {
                HAL_Delay(5);
            }
        }
    }

    return result;
}

/**
 * \brief   Configure the LIS3DSH with the given configuration parameters.
 * \param   config  Configuration struct for LIS3DSH.
 * \returns True if the LIS3DSH could be configured successfully, else false.
 * \note    The intended use is fifo mode: stream.
 * \note    AN3393 - LIS3DSH - Application note - 10.3.1 Bypass mode - last few lines.
 */
bool LIS3DSH::Configure(const IConfig& config)
{
    const Config& cfg = reinterpret_cast<const Config&>(config);

    uint8_t ODR    = GetSampleFrequencyAsODR(cfg.mSampleFrequency);
    uint8_t FSCALE = GetScaleAsFCALE(cfg.mScale);
    uint8_t BW     = GetAntiAliasingFilterAsBW(cfg.mAntiAliasingFilter);

    uint8_t src = (ODR | (BDU << 3) | AXES_ENABLED);    // Set sample frequency, all axes enabled, not using BDU
    bool result = WriteRegister(CTRL_REG4, &src, 1);
    EXPECT(result);

    result &= PrepareReadBuffer(cfg.mSampleFrequency);
    EXPECT(result);

    src = 0x68;                                         // INT1 enabled, active high, pulsed
    result &= WriteRegister(CTRL_REG3, &src, 1);
    EXPECT(result);

    src = (BW | FSCALE);                                // Default: anti-aliasing 200 Hz, +/- 2g
    result &= WriteRegister(CTRL_REG5, &src, 1);
    EXPECT(result);

    src = 0x54;                                         // FIFO enabled, watermark on INT1
    result &= WriteRegister(CTRL_REG6, &src, 1);
    EXPECT(result);

    // Leave fifo in 'bypass' mode: setting another mode enables acquisition.
    EXPECT(WATERMARK_LEVEL <= 32);                      // Fifo only 32 samples big
    src = WATERMARK_LEVEL;                              // FIFO mode (disabled), watermark level (default 25 samples X,Y,Z)
    result &= WriteRegister(FIFO_CTRL, &src, 1);
    EXPECT(result);

    return result;
}

/**
 * \brief   Prepare the read buffer by claiming memory on the heap to store
 *          the read fifo data into.
 * \param   sampleFrequency     Sample frequency for accelerometer data.
 * \returns True if the read buffer could be prepared successfully, else false.
 */
bool LIS3DSH::PrepareReadBuffer(SampleFrequency sampleFrequency)
{
    // If we had claimed memory before: delete it
    if (mReadBuffer != nullptr) { delete [] mReadBuffer; }

    // Claim new segment in heap memory to retrieve sample data.
    mReadBuffer = new(std::nothrow) uint8_t[READ_BUFFER_SIZE];

    // Clear buffer: fill with 0
    if (mReadBuffer != nullptr)
    {
        std::fill_n(mReadBuffer, READ_BUFFER_SIZE, 0);
        return true;
    }
    return false;
}

/**
 * \brief   Clears the fifo in the LIS3DSH.
 * \details This is done by checking the fifo status and number of bytes left
 *          in the fifo. Skips clearing if fifo is empty.
 * \returns True if the fifo could be cleared successfully (or already empty),
 *          else false.
 */
bool LIS3DSH::ClearFifo()
{
    uint8_t dest = 0;
    bool result = ReadRegister(FIFO_SRC, &dest, 1);
    EXPECT(result);

    if ( ! (dest & FIFO_EMPTY) )
    {
        uint8_t nrSamplesInFifo = dest & 0x1F;
        if (nrSamplesInFifo > 0)
        {
            const uint8_t length = 3 * 2 * nrSamplesInFifo;   // X,Y,Z * int16_t * samples in fifo
            uint8_t samples[length] = {};
            result &= ReadRegister(OUT_X_L, samples, length);
            EXPECT(result);
        }

        dest = 0;
        result &= ReadRegister(FIFO_SRC, &dest, 1);
        EXPECT(result);

        if ( ! (dest & FIFO_EMPTY) )
        {
            const uint8_t length = 3 * 2;                       // X,Y,Z * int16_t * 1 sample in fifo
            uint8_t samples[length] = {};
            result &= ReadRegister(OUT_X_L, samples, length);
            EXPECT(result);
        }
    }
    return result;
}

/**
 * \brief   Return the sample frequency as ODR setting in register, CTRL_REG4.
 * \param   sampleFrequency     Sample frequency for accelerometer data.
 * \returns The ODR setting with the sample frequency for CTRL_REG4 register.
 */
uint8_t LIS3DSH::GetSampleFrequencyAsODR(SampleFrequency sampleFrequency)
{
    uint8_t odrValue = 0;

    switch (sampleFrequency)
    {
        case SampleFrequency::_3_125_Hz: odrValue = 0x10; break;
        case SampleFrequency::_6_25_Hz:  odrValue = 0x20; break;
        case SampleFrequency::_12_5_Hz:  odrValue = 0x30; break;
        case SampleFrequency::_25_Hz:    odrValue = 0x40; break;
        case SampleFrequency::_50_Hz:    odrValue = 0x50; break;
        case SampleFrequency::_100_Hz:   odrValue = 0x60; break;
        case SampleFrequency::_400_Hz:   odrValue = 0x70; break;
        case SampleFrequency::_800_Hz:   odrValue = 0x80; break;
        case SampleFrequency::_1600_Hz:  odrValue = 0x90; break;
    }

    return odrValue;
}

/**
 * \brief   Return the sample frequency as FSCALE setting in register, CTRL_REG5.
 * \param   scale   Scale of the accelerometer data.
 * \returns The FSCALE setting with the scale for CTRL_REG5 register.
 */
uint8_t LIS3DSH::GetScaleAsFCALE(Scale scale)
{
    uint8_t fscaleVal = 0;

    switch (scale)
    {
        case Scale::_2_G:  fscaleVal = 0x00; break;
        case Scale::_4_G:  fscaleVal = 0x08; break;
        case Scale::_6_G:  fscaleVal = 0x10; break;
        case Scale::_8_G:  fscaleVal = 0x18; break;
        case Scale::_16_G: fscaleVal = 0x20; break;
    }

    return fscaleVal;
}

/**
 * \brief   Return the sample frequency as BW setting in register, CTRL_REG5.
 * \param   antiAliasingFilter  Anti-aliasing filter bandwidth.
 * \returns The BW setting with the anti-aliasing filter for CTRL_REG5 register.
 */
uint8_t LIS3DSH::GetAntiAliasingFilterAsBW(AntiAliasingFilter antiAliasingFilter)
{
    uint8_t bwVal = 0;

    switch (antiAliasingFilter)
    {
        case AntiAliasingFilter::_50_Hz:  bwVal = 0xC0; break;
        case AntiAliasingFilter::_200_Hz: bwVal = 0x40; break;
        case AntiAliasingFilter::_400_Hz: bwVal = 0x80; break;
        case AntiAliasingFilter::_800_Hz: bwVal = 0x00; break;
    }

    return bwVal;
}

/**
 * \brief   Return the fifo mode FMODE setting in register, FIFO_CTRL.
 * \param   fifoMode    Fifo mode.
 * \returns The FMODE setting with the fifo mode for FIFO_CTRL register.
 */
uint8_t LIS3DSH::GetFifoModeAsFMODE(FifoMode fifoMode)
{
    uint8_t fmodeVal = 0;

    switch (fifoMode)
    {
        case FifoMode::Bypass:           fmodeVal = 0x00; break;
        case FifoMode::Fifo:             fmodeVal = 0x20; break;
        case FifoMode::Stream:           fmodeVal = 0x40; break;
        case FifoMode::StreamThenFifo:   fmodeVal = 0x60; break;
        case FifoMode::BypassThenStream: fmodeVal = 0x80; break;
        case FifoMode::BypassThenFifo:   fmodeVal = 0xE0; break;
    }

    return fmodeVal;
}

/**
 * \brief   Handler for read fifo done event.
 * \details Sets ChipSelect high, then calls handler for data available event.
 */
void LIS3DSH::ReadAxesCompleted()
{
    mChipSelect.Set(Level::HIGH);

    if (mHandler)
    {
        mHandler(READ_BUFFER_SIZE);
    }
}

/**
 * \brief   Helper method to write a register in the LIS3DSH.
 * \param   reg     The register to write.
 * \param   src     Pointer to the data to write.
 * \param   length  Length of the data to write in bytes.
 * \returns True if the register is written successfully, else false.
 * \note    Asserts if src is nullptr or length invalid.
 */
bool LIS3DSH::WriteRegister(uint8_t reg, const uint8_t* src, uint16_t length)
{
    EXPECT(src);
    EXPECT(length > 0);

    mChipSelect.Set(Level::LOW);
    bool result = mSpi.WriteBlocking(&reg, 1);
    EXPECT(result);
    if (result)
    {
        result &= mSpi.WriteBlocking(src, length);
        EXPECT(result);
    }
    mChipSelect.Set(Level::HIGH);

    return result;
}

/**
 * \brief   Helper method to read a register from the LIS3DSH.
 * \param   reg     The register to read.
 * \param   dest    Pointer to the buffer to store read data into.
 * \param   length  Length of the data to read in bytes.
 * \returns True if the register is read successfully, else false.
 * \note    Asserts if dest is nullptr or length invalid.
 */
bool LIS3DSH::ReadRegister(uint8_t reg, uint8_t* dest, uint16_t length)
{
    EXPECT(dest);
    EXPECT(length > 0);

    reg = reg | READ_MASK;

    mChipSelect.Set(Level::LOW);
    bool result = mSpi.WriteBlocking(&reg, 1);
    EXPECT(result);
    if (result)
    {
        result &= mSpi.ReadBlocking(dest, length);
        EXPECT(result);
    }
    mChipSelect.Set(Level::HIGH);

    return result;
}

/**
 * \brief   INT1 pin interrupt handler.
 * \details Starts reading data from LIS3DSH fifo into read buffer asynchronously.
 */
void LIS3DSH::CallbackInt1()
{
    if ((mReadBuffer != nullptr) && (READ_BUFFER_SIZE > 0))
    {
        uint8_t reg = (OUT_X_L | READ_MASK);

        mChipSelect.Set(Level::LOW);
        bool result = mSpi.WriteBlocking(&reg, 1);
        EXPECT(result);
        if (result)
        {
            result &= mSpi.ReadDMA(mReadBuffer, READ_BUFFER_SIZE, [this]() { this->ReadAxesCompleted(); } );
            EXPECT(result);
        }
        else
        {
            mChipSelect.Set(Level::HIGH);
        }
    }
}

/**
 * \brief   INT2 pin interrupt handler.
 */
void LIS3DSH::CallbackInt2()
{
    __NOP();    // Not used yet...
}
