/**
 * \file    stm32f4xx_hal_dac.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL DAC surface for the unit-test build.
 *          Declares the channel-config struct, the DAC channel / trigger /
 *          alignment / output-buffer macros, the RCC DAC clock gating macros
 *          and the HAL_DAC_* prototypes the real Dac driver compiles against.
 *          The DAC types/instance themselves live in the umbrella
 *          stm32f4xx_hal.h (Dac.hpp pulls in only the umbrella).
 *
 * \details Every HAL_DAC_* entry point returns HAL_OK by default; the
 *          FakeDAC_Set*Result hooks drive each failure branch independently
 *          (Init, DeInit, ConfigChannel, Start, SetValue, Start_DMA). The
 *          Stop / Stop_DMA bodies are no-ops (the driver ignores their result).
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_DAC_H
#define __STM32F4xx_HAL_DAC_H


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include "stm32f4xx_hal.h"


#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \brief   DAC channel configuration -- field names mirror the real HAL so
 *          ConfigureChannel populates it exactly as on hardware.
 */
typedef struct
{
    uint32_t DAC_Trigger;        ///< DAC_TRIGGER_*
    uint32_t DAC_OutputBuffer;   ///< DAC_OUTPUTBUFFER_*
} DAC_ChannelConfTypeDef;


/************************************************************************/
/* Config-value macros (mirror the real HAL)                            */
/************************************************************************/
#define DAC_CHANNEL_1               ((uint32_t)0x00000000U)
#define DAC_CHANNEL_2               ((uint32_t)0x00000010U)

#define DAC_TRIGGER_NONE            ((uint32_t)0x00000000U)
#define DAC_TRIGGER_T2_TRGO         ((uint32_t)0x0000002CU)
#define DAC_TRIGGER_T4_TRGO         ((uint32_t)0x0000003CU)
#define DAC_TRIGGER_T5_TRGO         ((uint32_t)0x0000001CU)
#define DAC_TRIGGER_T6_TRGO         ((uint32_t)0x00000004U)
#define DAC_TRIGGER_T7_TRGO         ((uint32_t)0x00000014U)
#define DAC_TRIGGER_T8_TRGO         ((uint32_t)0x0000000CU)
#define DAC_TRIGGER_EXT_IT9         ((uint32_t)0x00000034U)

#define DAC_ALIGN_12B_R             ((uint32_t)0x00000000U)
#define DAC_ALIGN_12B_L             ((uint32_t)0x00000004U)
#define DAC_ALIGN_8B_R              ((uint32_t)0x00000008U)

#define DAC_OUTPUTBUFFER_ENABLE     ((uint32_t)0x00000000U)
#define DAC_OUTPUTBUFFER_DISABLE    ((uint32_t)0x00000002U)


/************************************************************************/
/* RCC DAC clock gating -- pretend the clock is already enabled.        */
/************************************************************************/
#define __HAL_RCC_DAC_CLK_ENABLE()     do { } while(0)
#define __HAL_RCC_DAC_CLK_DISABLE()    do { } while(0)


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_dac.cpp)          */
/************************************************************************/
HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef* hdac);
HAL_StatusTypeDef HAL_DAC_DeInit(DAC_HandleTypeDef* hdac);
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef* hdac, DAC_ChannelConfTypeDef* sConfig, uint32_t Channel);
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef* hdac, uint32_t Channel, uint32_t Alignment, uint32_t Data);
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef* hdac, uint32_t Channel);
HAL_StatusTypeDef HAL_DAC_Stop(DAC_HandleTypeDef* hdac, uint32_t Channel);
HAL_StatusTypeDef HAL_DAC_Start_DMA(DAC_HandleTypeDef* hdac, uint32_t Channel, uint32_t* pData, uint32_t Length, uint32_t Alignment);
HAL_StatusTypeDef HAL_DAC_Stop_DMA(DAC_HandleTypeDef* hdac, uint32_t Channel);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake DAC state: every result back to HAL_OK. */
void FakeDAC_Reset(void);

void FakeDAC_SetInitResult(HAL_StatusTypeDef result);          ///< Drives Init failure
void FakeDAC_SetDeInitResult(HAL_StatusTypeDef result);        ///< Drives Sleep failure
void FakeDAC_SetConfigChannelResult(HAL_StatusTypeDef result); ///< Drives ConfigureChannel failure
void FakeDAC_SetStartResult(HAL_StatusTypeDef result);         ///< Drives StartChannel (SetValue) failure
void FakeDAC_SetSetValueResult(HAL_StatusTypeDef result);      ///< Drives SetValue write failure
void FakeDAC_SetStartDmaResult(HAL_StatusTypeDef result);      ///< Drives StartWaveform failure


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_DAC_H
