/**
 * \file     config.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   General configuration file for ExampleProject.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/ExampleProject/target/Src
 *
 * \details Intended use is to provide an example how to configure the ASSERT
 *          and EXPECT macros.
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2021
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#ifdef __cplusplus
  extern "C" {
#endif


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include <stddef.h>


// Version
static const char versionString[] = "My Embedded FreeRTOS Project v0.1";


// Available Expect/Assert modes
#define HANDLE_BY_IGNORING     1
#define HANDLE_BY_LOGGING      2
#define HANDLE_BY_BREAKPOINT   3
#define HANDLE_BY_RESETTING    4

// Configuration of the EXPECT/ASSERT macros
#define EXPECT_MODE            HANDLE_BY_LOGGING
#define ASSERT_MODE            HANDLE_BY_RESETTING


// Available LIS3DSH accelerometer settings
#define REAL_LIS3DSH           1
#define SIMULATED_LIS3DSH      2

// Configuration of the LIS3DSH accelerometer
#define LIS3DSH_ACCELEROMETER  REAL_LIS3DSH


// Available simulated sensor output data
#define SAWTOOTH_SIGNAL        1
#define SEMI_REAL_SIGNAL       2

// Configuration of the simulated sensor output data
#define SIMULATED_SENSOR_OUTPUT_DATA    SAWTOOTH_SIGNAL


#ifdef __cplusplus
}
#endif

#endif  // CONFIG_H_
