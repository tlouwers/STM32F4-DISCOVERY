/**
 * \file    stm32f4xx_hal_adc.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL ADC surface for the unit-test build.
 *          Declares the channel-config struct, the ADC resolution / prescaler /
 *          channel / sampling-time / data-align / trigger macros, the RCC ADC
 *          clock gating macros and the HAL_ADC_* prototypes the real Adc driver
 *          compiles against. The ADC types/instances themselves live in the
 *          umbrella stm32f4xx_hal.h (Adc.hpp pulls in only the umbrella).
 *
 * \details Every HAL_ADC_* entry point returns HAL_OK by default; the
 *          FakeADC_Set*Result hooks drive each failure branch independently
 *          (Init, DeInit, ConfigChannel, Start, Stop, PollForConversion,
 *          Start_IT). HAL_ADC_GetValue returns a controllable conversion value
 *          (FakeADC_SetConversionValue). HAL_ADC_IRQHandler counts its
 *          invocations and, having reached the driver via the registered
 *          callbackIRQ, drives the driver's HAL_ADC_ConvCpltCallback override so
 *          the end-of-conversion dispatch path can be exercised natively.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_ADC_H
#define __STM32F4xx_HAL_ADC_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* FunctionalState (mirror the real HAL)                                */
/************************************************************************/
/* The real HAL defines DISABLE/ENABLE in stm32f4xx.h; the Adc driver assigns
   DISABLE to the single-conversion handle fields. Only this header (included
   solely by Adc.cpp / TestAdc.cpp) provides it for the native build. */
#ifndef HAL_FUNCTIONALSTATE_DEFINED
#define HAL_FUNCTIONALSTATE_DEFINED
typedef enum
{
    DISABLE = 0U,
    ENABLE  = 1U
} FunctionalState;
#endif

#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY               ((uint32_t)0xFFFFFFFFU)
#endif


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \brief   ADC regular-channel configuration -- field names mirror the real
 *          HAL so Adc::Init populates it exactly as on hardware.
 */
typedef struct
{
    uint32_t Channel;        ///< ADC_CHANNEL_*
    uint32_t Rank;           ///< Conversion rank in the sequence
    uint32_t SamplingTime;   ///< ADC_SAMPLETIME_*CYCLES
    uint32_t Offset;         ///< Conversion offset (unused on F4 regular path)
} ADC_ChannelConfTypeDef;


/************************************************************************/
/* Config-value macros (mirror the real HAL)                            */
/************************************************************************/
#define ADC_DATAALIGN_RIGHT                 ((uint32_t)0x00000000U)
#define ADC_DATAALIGN_LEFT                  ((uint32_t)0x00000800U)

#define ADC_EOC_SEQ_CONV                    ((uint32_t)0x00000000U)
#define ADC_EOC_SINGLE_CONV                 ((uint32_t)0x00000001U)

#define ADC_SOFTWARE_START                  ((uint32_t)0x0000000FU)
#define ADC_EXTERNALTRIGCONVEDGE_NONE       ((uint32_t)0x00000000U)

#define ADC_RESOLUTION_12B                  ((uint32_t)0x00000000U)
#define ADC_RESOLUTION_10B                  ((uint32_t)0x01000000U)
#define ADC_RESOLUTION_8B                   ((uint32_t)0x02000000U)
#define ADC_RESOLUTION_6B                   ((uint32_t)0x03000000U)

#define ADC_CLOCK_SYNC_PCLK_DIV2            ((uint32_t)0x00000000U)
#define ADC_CLOCK_SYNC_PCLK_DIV4            ((uint32_t)0x00010000U)
#define ADC_CLOCK_SYNC_PCLK_DIV6            ((uint32_t)0x00020000U)
#define ADC_CLOCK_SYNC_PCLK_DIV8            ((uint32_t)0x00030000U)

#define ADC_CHANNEL_0                       ((uint32_t)0x00000000U)
#define ADC_CHANNEL_1                       ((uint32_t)0x00000001U)
#define ADC_CHANNEL_2                       ((uint32_t)0x00000002U)
#define ADC_CHANNEL_3                       ((uint32_t)0x00000003U)
#define ADC_CHANNEL_4                       ((uint32_t)0x00000004U)
#define ADC_CHANNEL_5                       ((uint32_t)0x00000005U)
#define ADC_CHANNEL_6                       ((uint32_t)0x00000006U)
#define ADC_CHANNEL_7                       ((uint32_t)0x00000007U)
#define ADC_CHANNEL_8                       ((uint32_t)0x00000008U)
#define ADC_CHANNEL_9                       ((uint32_t)0x00000009U)
#define ADC_CHANNEL_10                      ((uint32_t)0x0000000AU)
#define ADC_CHANNEL_11                      ((uint32_t)0x0000000BU)
#define ADC_CHANNEL_12                      ((uint32_t)0x0000000CU)
#define ADC_CHANNEL_13                      ((uint32_t)0x0000000DU)
#define ADC_CHANNEL_14                      ((uint32_t)0x0000000EU)
#define ADC_CHANNEL_15                      ((uint32_t)0x0000000FU)

#define ADC_SAMPLETIME_3CYCLES              ((uint32_t)0x00000000U)
#define ADC_SAMPLETIME_15CYCLES             ((uint32_t)0x00000001U)
#define ADC_SAMPLETIME_28CYCLES             ((uint32_t)0x00000002U)
#define ADC_SAMPLETIME_56CYCLES             ((uint32_t)0x00000003U)
#define ADC_SAMPLETIME_84CYCLES             ((uint32_t)0x00000004U)
#define ADC_SAMPLETIME_112CYCLES            ((uint32_t)0x00000005U)
#define ADC_SAMPLETIME_144CYCLES            ((uint32_t)0x00000006U)
#define ADC_SAMPLETIME_480CYCLES            ((uint32_t)0x00000007U)


/************************************************************************/
/* RCC ADC clock gating -- pretend the clock is already enabled.        */
/************************************************************************/
#define __HAL_RCC_ADC1_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_ADC2_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_ADC3_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_ADC1_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_ADC2_CLK_DISABLE()   do { } while(0)
#define __HAL_RCC_ADC3_CLK_DISABLE()   do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_adc.cpp)          */
/************************************************************************/
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
uint32_t          HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc);
void              HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc);

/* Defined by the driver (Adc.cpp); declared here so the definition links with
   C linkage and the fake HAL_ADC_IRQHandler can dispatch into it. */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake ADC state: every result back to HAL_OK, value + counters to 0. */
void FakeADC_Reset(void);

void FakeADC_SetInitResult(HAL_StatusTypeDef result);          ///< Drives Init failure
void FakeADC_SetDeInitResult(HAL_StatusTypeDef result);        ///< Drives Sleep failure
void FakeADC_SetConfigChannelResult(HAL_StatusTypeDef result); ///< Drives Init channel-config failure
void FakeADC_SetStartResult(HAL_StatusTypeDef result);         ///< Drives GetValue start failure
void FakeADC_SetStopResult(HAL_StatusTypeDef result);          ///< Drives GetValue final-stop failure
void FakeADC_SetPollResult(HAL_StatusTypeDef result);          ///< Drives GetValue poll failure
void FakeADC_SetStartItResult(HAL_StatusTypeDef result);       ///< Drives GetValueInterrupt failure
void FakeADC_SetConversionValue(uint32_t value);               ///< Value returned by HAL_ADC_GetValue
int  FakeADC_IRQHandlerCallCount(void);                        ///< HAL_ADC_IRQHandler invocation count


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_ADC_H
