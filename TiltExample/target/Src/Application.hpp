/**
 * \file     Application.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Main application file for FreeRTOS demo.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/FreeRTOSProject/target/Src
 *
 * \details Intended use is to provide an example how to read the accelerometer.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    12-2021
 */

#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include "config.h"
#include "components/HI-M1388AR/HI-M1388AR.hpp"
#include "components/HI-M1388AR/FakeHI-M1388AR.hpp"
#include "components/LIS3DSH/LIS3DSH.hpp"
#include "components/LIS3DSH/FakeLIS3DSH.hpp"
#include "drivers/DMA/DMA.hpp"
#include "drivers/Pin/Pin.hpp"
#include "drivers/SPI/SPI.hpp"
#include "drivers/Usart/Usart.hpp"


/************************************************************************/
/* Structs                                                              */
/************************************************************************/
/**
 * \struct  MotionSampleRaw
 * \brief   Raw motion sensor values.
 */
struct MotionSampleRaw
{
    int16_t X;  ///< Raw sensor X value
    int16_t Y;  ///< Raw sensor Y value
    int16_t Z;  ///< Raw sensor Z value
};

/**
 * \struct  MotionSample
 * \brief   Motion sensor values in G's (m/s2), pitch and roll in degrees.
 */
struct MotionSample
{
    float X;        ///< Sensor value X in G (m/s2)
    float Y;        ///< Sensor value Y in G (m/s2)
    float Z;        ///< Sensor value Z in G (m/s2)
    float pitch;    ///< Sensor pitch value in degrees
    float roll;     ///< Sensor roll value in degrees
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   Main application class.
 */
class Application
{
public:
    Application();
    virtual ~Application() {};

    bool Init();
    void Error();

    bool CreateTasks();
    void StartTasks();

private:
    Pin mLedGreen;
    Pin mLedOrange;
    Pin mLedRed;
    Pin mLedBlue;
    Pin mChipSelectMatrix;
    Pin mChipSelectMotion;
    Pin mMotionInt1;
    Pin mMotionInt2;

    SPI   mSPIMotion;
    SPI   mSPIMatrix;
    Usart mUsart;

    DMA mDMA_SPI_Tx;
    DMA mDMA_SPI_Rx;

#if (HI_M1388AR_DISPLAY == REAL_HI_M1388AR)
    HI_M1388AR     mMatrix;
#else
    FakeHI_M1388AR mMatrix;
#endif

#if (LIS3DSH_ACCELEROMETER == REAL_LIS3DSH)
    LIS3DSH        mLIS3DSH;
#else
    FakeLIS3DSH    mLIS3DSH;
#endif

    std::atomic<uint8_t> mMotionLength;

    void MotionDataReceived(uint8_t length);

    MotionSample CalculateMotionSample(const MotionSampleRaw &sampleRaw);
    void CalculatePixel(uint8_t *dest, const MotionSample &sample, bool invert = false);

    void CallbackMotionDataReceived();
    void CallbackUpdateDisplay(const MotionSample &sample);
    void CallbackSendSampleViaUsart(const MotionSampleRaw &sample);
};


#endif  // APPLICATION_HPP_
