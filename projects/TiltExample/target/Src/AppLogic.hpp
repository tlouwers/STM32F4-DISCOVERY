/**
 * \file     AppLogic.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \class   AppLogic
 *
 * \brief   Interface-injected application logic for the tilt example.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/TiltExample/target/Src
 *
 * \details Holds the testable behaviour split out of Application: the pure
 *          motion-to-pitch/roll conversion, the pitch/roll-to-pixel mapping
 *          and the motion read + tilt render. It depends only on driver
 *          interfaces (ILIS3DSH, IPin, IHI_M1388AR) so it can be unit-tested
 *          with Mock drivers, while Application remains the thin composition
 *          root that owns the FreeRTOS task/queue plumbing and USART output.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef APPLOGIC_HPP_
#define APPLOGIC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <atomic>
#include <cstdint>
#include "interfaces/ILIS3DSH.hpp"
#include "interfaces/IPin.hpp"
#include "interfaces/IHI_M1388AR.hpp"


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
 * \brief   Testable application logic, injected with driver interfaces.
 */
class AppLogic final
{
public:
    AppLogic(ILIS3DSH& accelerometer, IPin& motionLed, IHI_M1388AR& matrix);

    void OnMotionData(uint8_t length);
    bool RetrieveMotion(uint8_t* dest, uint8_t& lengthOut);

    MotionSample CalculateMotionSample(const MotionSampleRaw& sampleRaw) const;
    void CalculatePixel(uint8_t* dest, const MotionSample& sample, bool invert = false) const;

    void RenderTilt(const MotionSample& sample);

private:
    static void ReverseBytes(uint8_t* start, size_t size);

    ILIS3DSH&    mAccelerometer;
    IPin&        mMotionLed;
    IHI_M1388AR& mMatrix;

    std::atomic<uint8_t> mMotionLength;
};


#endif  // APPLOGIC_HPP_
