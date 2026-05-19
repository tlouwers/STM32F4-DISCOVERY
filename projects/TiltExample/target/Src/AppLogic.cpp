/**
 * \file    AppLogic.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Interface-injected application logic for the tilt example.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/TiltExample/target/Src
 *
 * \details Holds the testable behaviour split out of Application: the pure
 *          motion conversion and pixel mapping, the motion read and the
 *          tilt render.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <math.h>
#include <cstddef>
#include "AppLogic.hpp"
#include "utility/Assert/Assert.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
// Perform scaling --> (4000/65535) milli-G per digit for +/-2g full scale when using the 16-bit output
static constexpr float K = 4.0 / UINT16_MAX;        // K expressed in G (m/s2), not milli-G

// Half-circle in degrees, expressed against PI (M_PI is not ISO C++14)
static constexpr double PI = 3.14159265358979323846;

// 8x8 Led Matrix display
static constexpr uint8_t MATRIX_NR_COLUMNS = 8;
static constexpr uint8_t MATRIX_NR_ROWS    = 8;


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Constructor, binds the injected driver interfaces.
 * \param   accelerometer   Accelerometer used to retrieve motion samples.
 * \param   motionLed       Led toggled when motion data is processed.
 * \param   matrix          8x8 led matrix used to render the tilt.
 */
AppLogic::AppLogic(ILIS3DSH& accelerometer, IPin& motionLed, IHI_M1388AR& matrix) :
    mAccelerometer(accelerometer),
    mMotionLed(motionLed),
    mMatrix(matrix),
    mMotionLength(0)
{ ; }

/**
 * \brief   Handler for the motion-data-received event. Records the reported
 *          sample length so the motion task can drain it later.
 * \param   length  The number of samples reported by the accelerometer.
 * \note    Called from ISR context in the composition root.
 */
void AppLogic::OnMotionData(uint8_t length)
{
    mMotionLength = length;
}

/**
 * \brief   Drains pending motion data (if any) into the caller-supplied
 *          buffer, giving a visual indication while doing so.
 * \param   dest        Destination buffer for the raw axes data.
 * \param   lengthOut   Receives the number of bytes reported by the sensor.
 * \returns True if motion data was pending (and drained), else false.
 */
bool AppLogic::RetrieveMotion(uint8_t* dest, uint8_t& lengthOut)
{
    lengthOut = 0;

    if (mMotionLength > 0)
    {
        mMotionLed.Toggle();

        uint8_t length = mMotionLength;
        bool retrieveResult = mAccelerometer.RetrieveAxesData(dest, length);
        EXPECT(retrieveResult);
        (void)(retrieveResult);

        lengthOut = length;
        return true;
    }

    return false;
}

/**
 * \brief   Convert to G (m/s2) and calculate the pitch and roll.
 * \param   sampleRaw   The motion sample to convert.
 * \returns Converted motion sample.
 */
MotionSample AppLogic::CalculateMotionSample(const MotionSampleRaw& sampleRaw) const
{
    MotionSample sample;

    // Convert to G (m/s2)
    sample.X = sampleRaw.X * K;
    sample.Y = sampleRaw.Y * K;
    sample.Z = sampleRaw.Z * K;

    // Calculate the pitch and roll in degrees
    sample.pitch = 180.0f * atan2f(sample.Y, sample.Z) / static_cast<float>(PI);
    sample.roll  = 180.0f * atan2f(sample.X, sample.Z) / static_cast<float>(PI);

    return sample;
}

/**
 * \brief   Calculate the pixel to display on the matrix display based upon the
 *          pitch and roll.
 * \param   dest    The destination pixel array (8 bytes) to fill.
 * \param   sample  Motion sample with pitch and roll in degrees.
 * \param   invert  Flag, indicate if the display should be inverted or not.
 */
void AppLogic::CalculatePixel(uint8_t* dest, const MotionSample& sample, bool invert /* = false */) const
{
    if (dest != nullptr)
    {
        uint8_t columnPitch = 0;
             if (sample.pitch >  40.0f) { columnPitch = 0x0F; }     // Special case: row
        else if (sample.pitch >  30.0f) { columnPitch = 7;    }
        else if (sample.pitch >  20.0f) { columnPitch = 6;    }
        else if (sample.pitch >  10.0f) { columnPitch = 5;    }
        else if (sample.pitch >=  0.0f) { columnPitch = 4;    }
        else if (sample.pitch > -10.0f) { columnPitch = 3;    }
        else if (sample.pitch > -20.0f) { columnPitch = 2;    }
        else if (sample.pitch > -30.0f) { columnPitch = 1;    }
        else if (sample.pitch > -40.0f) { columnPitch = 0;    }
        else                            { columnPitch = 0xF0; }     // Special case: row

        uint8_t rowRoll = 0;
             if (sample.roll >  40.0f) { rowRoll = 0x0F; }          // Special case: column
        else if (sample.roll >  30.0f) { rowRoll = 7;    }
        else if (sample.roll >  20.0f) { rowRoll = 6;    }
        else if (sample.roll >  10.0f) { rowRoll = 5;    }
        else if (sample.roll >=  0.0f) { rowRoll = 4;    }
        else if (sample.roll > -10.0f) { rowRoll = 3;    }
        else if (sample.roll > -20.0f) { rowRoll = 2;    }
        else if (sample.roll > -30.0f) { rowRoll = 1;    }
        else if (sample.roll > -40.0f) { rowRoll = 0;    }
        else                           { rowRoll = 0xF0; }          // Special case: column

        uint8_t pixel = 0;
             if (columnPitch == 0x0F) { for (uint8_t i = 0; i < MATRIX_NR_COLUMNS; i++ ) { dest[i] = 0x80; } }
        else if (columnPitch == 0xF0) { for (uint8_t i = 0; i < MATRIX_NR_COLUMNS; i++ ) { dest[i] = 0x01; } }
        else                          { pixel = (1 << columnPitch); }

             if (rowRoll == 0x0F) { dest[7] = 0xFF; }
        else if (rowRoll == 0xF0) { dest[0] = 0xFF; }
        else if (columnPitch != 0x0F && columnPitch != 0xF0) { dest[rowRoll] = pixel; }

        if (invert)
        {
            ReverseBytes(dest, MATRIX_NR_ROWS);
        }
    }
}

/**
 * \brief   Render the tilt for the given sample onto the led matrix.
 * \param   sample  Motion sample with pitch and roll in degrees.
 */
void AppLogic::RenderTilt(const MotionSample& sample)
{
    uint8_t pixels[8] = {};

    CalculatePixel(pixels, sample, true);

    mMatrix.WriteDigits(pixels);
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Reverse a byte array.
 * \param   start   Pointer to first element of the byte array to reverse.
 * \param   size    Size of the array.
 */
void AppLogic::ReverseBytes(uint8_t* start, size_t size)
{
    if (start != nullptr)
    {
        uint8_t* lo = start;
        uint8_t* hi = start + size - 1;
        uint8_t swap;
        while (lo < hi) {
            swap = *lo;
            *lo++ = *hi;
            *hi-- = swap;
        }
    }
}
