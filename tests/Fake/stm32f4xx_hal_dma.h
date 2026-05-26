/**
 * \file    stm32f4xx_hal_dma.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native fake of the STM32F4 HAL DMA surface for the unit-test build.
 *          Declares the DMA types, register-value macros, stream instances and
 *          HAL_DMA_* prototypes that the real DMA driver (and the peripheral
 *          drivers that link a DMA stream via __HAL_LINKDMA) compile against.
 *
 * \details The macro values mirror the real HAL so a handle field inspected in
 *          a test carries its true meaning, but no behaviour depends on them:
 *          the fake HAL_DMA_Init returns HAL_OK (overridable via
 *          FakeDMA_SetInitResult) regardless of the configured fields. The
 *          stream instances are backed by real storage in the companion .cpp
 *          so the driver's `mHandle.Instance` is non-null and __HAL_DMA_*
 *          register macros have somewhere to write.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_DMA_H
#define __STM32F4xx_HAL_DMA_H


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
 * \brief   DMA stream register block. Only CR is modelled (so the
 *          __HAL_DMA_DISABLE_IT(.., DMA_IT_HT) reassert path is observable);
 *          the remaining registers are present for layout fidelity only.
 */
typedef struct
{
    volatile uint32_t CR;     ///< Stream configuration register
    volatile uint32_t NDTR;   ///< Stream number-of-data register
    volatile uint32_t PAR;    ///< Stream peripheral address register
    volatile uint32_t M0AR;   ///< Stream memory 0 address register
    volatile uint32_t M1AR;   ///< Stream memory 1 address register
    volatile uint32_t FCR;    ///< Stream FIFO control register
} DMA_Stream_TypeDef;

/**
 * \brief   DMA init struct -- the fields the driver populates in Configure().
 */
typedef struct
{
    uint32_t Channel;               ///< Channel selection (DMA_CHANNEL_x)
    uint32_t Direction;             ///< Transfer direction (DMA_xxx_TO_xxx)
    uint32_t PeriphInc;             ///< Peripheral increment (DMA_PINC_xxx)
    uint32_t MemInc;                ///< Memory increment (DMA_MINC_xxx)
    uint32_t PeriphDataAlignment;   ///< Peripheral data width (DMA_PDATAALIGN_xxx)
    uint32_t MemDataAlignment;      ///< Memory data width (DMA_MDATAALIGN_xxx)
    uint32_t Mode;                  ///< Normal / circular (DMA_NORMAL|DMA_CIRCULAR)
    uint32_t Priority;              ///< Stream priority (DMA_PRIORITY_xxx)
    uint32_t FIFOMode;              ///< FIFO mode (DMA_FIFOMODE_xxx)
    uint32_t FIFOThreshold;         ///< FIFO threshold (DMA_FIFO_THRESHOLD_xxx)
    uint32_t MemBurst;              ///< Memory burst (DMA_MBURST_xxx)
    uint32_t PeriphBurst;           ///< Peripheral burst (DMA_PBURST_xxx)
} DMA_InitTypeDef;

/**
 * \brief   DMA handle. `Parent` is written by __HAL_LINKDMA when a peripheral
 *          driver wires this stream into its hdmatx/hdmarx slot.
 */
typedef struct __DMA_HandleTypeDef
{
    DMA_Stream_TypeDef* Instance;   ///< Underlying stream
    DMA_InitTypeDef     Init;       ///< Configuration
    void*               Parent;     ///< Owning peripheral handle (set by __HAL_LINKDMA)
} DMA_HandleTypeDef;


/************************************************************************/
/* Register-value macros (mirror the real HAL)                          */
/************************************************************************/
#define DMA_CHANNEL_0               ((uint32_t)0x00000000U)
#define DMA_CHANNEL_1               ((uint32_t)0x02000000U)
#define DMA_CHANNEL_2               ((uint32_t)0x04000000U)
#define DMA_CHANNEL_3               ((uint32_t)0x06000000U)
#define DMA_CHANNEL_4               ((uint32_t)0x08000000U)
#define DMA_CHANNEL_5               ((uint32_t)0x0A000000U)
#define DMA_CHANNEL_6               ((uint32_t)0x0C000000U)
#define DMA_CHANNEL_7               ((uint32_t)0x0E000000U)
// NOTE: DMA_SxCR_CHSEL_3 is intentionally NOT defined (F407 has 8 channels);
// this keeps DMA::Channel8..15 #if'd out exactly as on the ARM build.

#define DMA_PERIPH_TO_MEMORY        ((uint32_t)0x00000000U)
#define DMA_MEMORY_TO_PERIPH        ((uint32_t)0x00000040U)
#define DMA_MEMORY_TO_MEMORY        ((uint32_t)0x00000080U)

#define DMA_PINC_DISABLE            ((uint32_t)0x00000000U)
#define DMA_PINC_ENABLE             ((uint32_t)0x00000200U)
#define DMA_MINC_ENABLE             ((uint32_t)0x00000400U)

#define DMA_PDATAALIGN_BYTE         ((uint32_t)0x00000000U)
#define DMA_PDATAALIGN_HALFWORD     ((uint32_t)0x00000800U)
#define DMA_PDATAALIGN_WORD         ((uint32_t)0x00001000U)

#define DMA_MDATAALIGN_BYTE         ((uint32_t)0x00000000U)
#define DMA_MDATAALIGN_HALFWORD     ((uint32_t)0x00002000U)
#define DMA_MDATAALIGN_WORD         ((uint32_t)0x00004000U)

#define DMA_NORMAL                  ((uint32_t)0x00000000U)
#define DMA_CIRCULAR                ((uint32_t)0x00000100U)

#define DMA_PRIORITY_LOW            ((uint32_t)0x00000000U)
#define DMA_PRIORITY_MEDIUM         ((uint32_t)0x00010000U)
#define DMA_PRIORITY_HIGH           ((uint32_t)0x00020000U)
#define DMA_PRIORITY_VERY_HIGH      ((uint32_t)0x00030000U)

#define DMA_FIFOMODE_DISABLE        ((uint32_t)0x00000000U)
#define DMA_FIFOMODE_ENABLE         ((uint32_t)0x00000004U)

#define DMA_FIFO_THRESHOLD_1QUARTERFULL     ((uint32_t)0x00000000U)
#define DMA_FIFO_THRESHOLD_HALFFULL         ((uint32_t)0x00000001U)
#define DMA_FIFO_THRESHOLD_3QUARTERSFULL    ((uint32_t)0x00000002U)
#define DMA_FIFO_THRESHOLD_FULL             ((uint32_t)0x00000003U)

#define DMA_MBURST_SINGLE           ((uint32_t)0x00000000U)
#define DMA_MBURST_INC4             ((uint32_t)0x00800000U)
#define DMA_MBURST_INC8             ((uint32_t)0x01000000U)
#define DMA_MBURST_INC16            ((uint32_t)0x01800000U)

#define DMA_PBURST_SINGLE           ((uint32_t)0x00000000U)
#define DMA_PBURST_INC4             ((uint32_t)0x00200000U)
#define DMA_PBURST_INC8             ((uint32_t)0x00400000U)
#define DMA_PBURST_INC16            ((uint32_t)0x00600000U)

#define DMA_IT_HT                   ((uint32_t)0x00000008U)   ///< Half-transfer interrupt enable (CR.HTIE)


/************************************************************************/
/* Control macros                                                       */
/************************************************************************/
/** Clear an interrupt-enable bit in the stream CR (modelled so the driver's
 *  HalfBufferInterrupt reassert is observable in a test). */
#define __HAL_DMA_DISABLE_IT(__HANDLE__, __INTERRUPT__) \
    (((DMA_Stream_TypeDef *)((__HANDLE__)->Instance))->CR &= ~(__INTERRUPT__))

/** Remaining transfer count of a DMA stream (NDTR). Read by the USART Rx-complete
 *  ISR to compute how many bytes actually arrived. */
#define __HAL_DMA_GET_COUNTER(__HANDLE__) \
    (((DMA_Stream_TypeDef *)((__HANDLE__)->Instance))->NDTR)

/** Wire a DMA handle into a peripheral handle's stream slot, and back-link. */
#define __HAL_LINKDMA(__HANDLE__, __PPP_DMA_FIELD__, __DMA_HANDLE__)   \
    do {                                                              \
        (__HANDLE__)->__PPP_DMA_FIELD__ = &(__DMA_HANDLE__);          \
        (__DMA_HANDLE__).Parent = (__HANDLE__);                       \
    } while(0)

/* RCC DMA clock gating -- pretend the clocks are already enabled. */
#define __HAL_RCC_DMA1_CLK_ENABLE()        do { } while(0)
#define __HAL_RCC_DMA2_CLK_ENABLE()        do { } while(0)
#define __HAL_RCC_DMA1_IS_CLK_DISABLED()   (0U)
#define __HAL_RCC_DMA2_IS_CLK_DISABLED()   (0U)


/************************************************************************/
/* Stream instances (backed by storage in the companion .cpp)           */
/************************************************************************/
extern DMA_Stream_TypeDef* const DMA1_Stream0;
extern DMA_Stream_TypeDef* const DMA1_Stream1;
extern DMA_Stream_TypeDef* const DMA1_Stream2;
extern DMA_Stream_TypeDef* const DMA1_Stream3;
extern DMA_Stream_TypeDef* const DMA1_Stream4;
extern DMA_Stream_TypeDef* const DMA1_Stream5;
extern DMA_Stream_TypeDef* const DMA1_Stream6;
extern DMA_Stream_TypeDef* const DMA1_Stream7;
extern DMA_Stream_TypeDef* const DMA2_Stream0;
extern DMA_Stream_TypeDef* const DMA2_Stream1;
extern DMA_Stream_TypeDef* const DMA2_Stream2;
extern DMA_Stream_TypeDef* const DMA2_Stream3;
extern DMA_Stream_TypeDef* const DMA2_Stream4;
extern DMA_Stream_TypeDef* const DMA2_Stream5;
extern DMA_Stream_TypeDef* const DMA2_Stream6;
extern DMA_Stream_TypeDef* const DMA2_Stream7;


/************************************************************************/
/* Driver-facing HAL surface (bodies in stm32f4xx_hal_dma.cpp)          */
/************************************************************************/
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef* hdma);
HAL_StatusTypeDef HAL_DMA_DeInit(DMA_HandleTypeDef* hdma);
void              HAL_DMA_IRQHandler(DMA_HandleTypeDef* hdma);


/************************************************************************/
/* Test-only observation / control hooks                                */
/************************************************************************/
/** Reset all fake DMA state: stream register storage, the forced Init result
 *  (back to HAL_OK) and the HAL_DMA_IRQHandler call counter. */
void FakeDMA_Reset(void);

/** Force the next (and subsequent) HAL_DMA_Init calls to return `result`,
 *  so the driver's Configure()-failure path can be exercised. */
void FakeDMA_SetInitResult(HAL_StatusTypeDef result);

/** Number of times HAL_DMA_IRQHandler has been invoked since the last reset
 *  -- proves the stream IRQ vector dispatched into the driver's Callback(). */
int FakeDMA_IRQHandlerCallCount(void);


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_DMA_H
