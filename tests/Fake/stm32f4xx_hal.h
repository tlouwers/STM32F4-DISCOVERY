/**
 * \file    stm32f4xx_hal.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Umbrella fake STM32F4 HAL header for the native unit-test build.
 *          Declares the minimum set of HAL types/macros/prototypes the real
 *          drivers depend on, so they compile and link without the vendor HAL.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests/Fake
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

#ifndef __STM32F4xx_HAL_H
#define __STM32F4xx_HAL_H


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief General Purpose I/O
 */
typedef struct
{
    volatile uint32_t MODER;    ///< GPIO port mode register,               Address offset: 0x00
    volatile uint32_t OTYPER;   ///< GPIO port output type register,        Address offset: 0x04
    volatile uint32_t OSPEEDR;  ///< GPIO port output speed register,       Address offset: 0x08
    volatile uint32_t PUPDR;    ///< GPIO port pull-up/pull-down register,  Address offset: 0x0C
    volatile uint32_t IDR;      ///< GPIO port input data register,         Address offset: 0x10
    volatile uint32_t ODR;      ///< GPIO port output data register,        Address offset: 0x14
    volatile uint32_t BSRR;     ///< GPIO port bit set/reset register,      Address offset: 0x18
    volatile uint32_t LCKR;     ///< GPIO port configuration lock register, Address offset: 0x1C
    volatile uint32_t AFR[2];   ///< GPIO alternate function registers,     Address offset: 0x20-0x24
} GPIO_TypeDef;

/**
 * @brief STM32F4XX Interrupt Number Definition, according to the selected device
 *        in @ref Library_configuration_section
 */
typedef enum
{
/******  Cortex-M4 Processor Exceptions Numbers ****************************************************************/
    NonMaskableInt_IRQn         = -14,    ///< 2 Non Maskable Interrupt
    MemoryManagement_IRQn       = -12,    ///< 4 Cortex-M4 Memory Management Interrupt
    BusFault_IRQn               = -11,    ///< 5 Cortex-M4 Bus Fault Interrupt
    UsageFault_IRQn             = -10,    ///< 6 Cortex-M4 Usage Fault Interrupt
    SVCall_IRQn                 = -5,     ///< 11 Cortex-M4 SV Call Interrupt
    DebugMonitor_IRQn           = -4,     ///< 12 Cortex-M4 Debug Monitor Interrupt
    PendSV_IRQn                 = -2,     ///< 14 Cortex-M4 Pend SV Interrupt
    SysTick_IRQn                = -1,     ///< 15 Cortex-M4 System Tick Interrupt
/******  STM32 specific Interrupt Numbers **********************************************************************/
    WWDG_IRQn                   = 0,      ///< Window WatchDog Interrupt
    PVD_IRQn                    = 1,      ///< PVD through EXTI Line detection Interrupt
    TAMP_STAMP_IRQn             = 2,      ///< Tamper and TimeStamp interrupts through the EXTI line
    RTC_WKUP_IRQn               = 3,      ///< RTC Wakeup interrupt through the EXTI line
    FLASH_IRQn                  = 4,      ///< FLASH global Interrupt
    RCC_IRQn                    = 5,      ///< RCC global Interrupt
    EXTI0_IRQn                  = 6,      ///< EXTI Line0 Interrupt
    EXTI1_IRQn                  = 7,      ///< EXTI Line1 Interrupt
    EXTI2_IRQn                  = 8,      ///< EXTI Line2 Interrupt
    EXTI3_IRQn                  = 9,      ///< EXTI Line3 Interrupt
    EXTI4_IRQn                  = 10,     ///< EXTI Line4 Interrupt
    DMA1_Stream0_IRQn           = 11,     ///< DMA1 Stream 0 global Interrupt
    DMA1_Stream1_IRQn           = 12,     ///< DMA1 Stream 1 global Interrupt
    DMA1_Stream2_IRQn           = 13,     ///< DMA1 Stream 2 global Interrupt
    DMA1_Stream3_IRQn           = 14,     ///< DMA1 Stream 3 global Interrupt
    DMA1_Stream4_IRQn           = 15,     ///< DMA1 Stream 4 global Interrupt
    DMA1_Stream5_IRQn           = 16,     ///< DMA1 Stream 5 global Interrupt
    DMA1_Stream6_IRQn           = 17,     ///< DMA1 Stream 6 global Interrupt
    ADC_IRQn                    = 18,     ///< ADC1, ADC2 and ADC3 global Interrupts
    CAN1_TX_IRQn                = 19,     ///< CAN1 TX Interrupt
    CAN1_RX0_IRQn               = 20,     ///< CAN1 RX0 Interrupt
    CAN1_RX1_IRQn               = 21,     ///< CAN1 RX1 Interrupt
    CAN1_SCE_IRQn               = 22,     ///< CAN1 SCE Interrupt
    EXTI9_5_IRQn                = 23,     ///< External Line[9:5] Interrupts
    TIM1_BRK_TIM9_IRQn          = 24,     ///< TIM1 Break interrupt and TIM9 global interrupt
    TIM1_UP_TIM10_IRQn          = 25,     ///< TIM1 Update Interrupt and TIM10 global interrupt
    TIM1_TRG_COM_TIM11_IRQn     = 26,     ///< TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt
    TIM1_CC_IRQn                = 27,     ///< TIM1 Capture Compare Interrupt
    TIM2_IRQn                   = 28,     ///< TIM2 global Interrupt
    TIM3_IRQn                   = 29,     ///< TIM3 global Interrupt
    TIM4_IRQn                   = 30,     ///< TIM4 global Interrupt
    I2C1_EV_IRQn                = 31,     ///< I2C1 Event Interrupt
    I2C1_ER_IRQn                = 32,     ///< I2C1 Error Interrupt
    I2C2_EV_IRQn                = 33,     ///< I2C2 Event Interrupt
    I2C2_ER_IRQn                = 34,     ///< I2C2 Error Interrupt
    SPI1_IRQn                   = 35,     ///< SPI1 global Interrupt
    SPI2_IRQn                   = 36,     ///< SPI2 global Interrupt
    USART1_IRQn                 = 37,     ///< USART1 global Interrupt
    USART2_IRQn                 = 38,     ///< USART2 global Interrupt
    USART3_IRQn                 = 39,     ///< USART3 global Interrupt
    EXTI15_10_IRQn              = 40,     ///< External Line[15:10] Interrupts
    RTC_Alarm_IRQn              = 41,     ///< RTC Alarm (A and B) through EXTI Line Interrupt
    OTG_FS_WKUP_IRQn            = 42,     ///< USB OTG FS Wakeup through EXTI line interrupt
    TIM8_BRK_TIM12_IRQn         = 43,     ///< TIM8 Break Interrupt and TIM12 global interrupt
    TIM8_UP_TIM13_IRQn          = 44,     ///< TIM8 Update Interrupt and TIM13 global interrupt
    TIM8_TRG_COM_TIM14_IRQn     = 45,     ///< TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt
    TIM8_CC_IRQn                = 46,     ///< TIM8 Capture Compare global interrupt
    DMA1_Stream7_IRQn           = 47,     ///< DMA1 Stream7 Interrupt
    FSMC_IRQn                   = 48,     ///< FSMC global Interrupt
    SDIO_IRQn                   = 49,     ///< SDIO global Interrupt
    TIM5_IRQn                   = 50,     ///< TIM5 global Interrupt
    SPI3_IRQn                   = 51,     ///< SPI3 global Interrupt
    UART4_IRQn                  = 52,     ///< UART4 global Interrupt
    UART5_IRQn                  = 53,     ///< UART5 global Interrupt
    TIM6_DAC_IRQn               = 54,     ///< TIM6 global and DAC1&2 underrun error  interrupts
    TIM7_IRQn                   = 55,     ///< TIM7 global interrupt
    DMA2_Stream0_IRQn           = 56,     ///< DMA2 Stream 0 global Interrupt
    DMA2_Stream1_IRQn           = 57,     ///< DMA2 Stream 1 global Interrupt
    DMA2_Stream2_IRQn           = 58,     ///< DMA2 Stream 2 global Interrupt
    DMA2_Stream3_IRQn           = 59,     ///< DMA2 Stream 3 global Interrupt
    DMA2_Stream4_IRQn           = 60,     ///< DMA2 Stream 4 global Interrupt
    ETH_IRQn                    = 61,     ///< Ethernet global Interrupt
    ETH_WKUP_IRQn               = 62,     ///< Ethernet Wakeup through EXTI line Interrupt
    CAN2_TX_IRQn                = 63,     ///< CAN2 TX Interrupt
    CAN2_RX0_IRQn               = 64,     ///< CAN2 RX0 Interrupt
    CAN2_RX1_IRQn               = 65,     ///< CAN2 RX1 Interrupt
    CAN2_SCE_IRQn               = 66,     ///< CAN2 SCE Interrupt
    OTG_FS_IRQn                 = 67,     ///< USB OTG FS global Interrupt
    DMA2_Stream5_IRQn           = 68,     ///< DMA2 Stream 5 global interrupt
    DMA2_Stream6_IRQn           = 69,     ///< DMA2 Stream 6 global interrupt
    DMA2_Stream7_IRQn           = 70,     ///< DMA2 Stream 7 global interrupt
    USART6_IRQn                 = 71,     ///< USART6 global interrupt
    I2C3_EV_IRQn                = 72,     ///< I2C3 event interrupt
    I2C3_ER_IRQn                = 73,     ///< I2C3 error interrupt
    OTG_HS_EP1_OUT_IRQn         = 74,     ///< USB OTG HS End Point 1 Out global interrupt
    OTG_HS_EP1_IN_IRQn          = 75,     ///< USB OTG HS End Point 1 In global interrupt
    OTG_HS_WKUP_IRQn            = 76,     ///< USB OTG HS Wakeup through EXTI interrupt
    OTG_HS_IRQn                 = 77,     ///< USB OTG HS global interrupt
    DCMI_IRQn                   = 78,     ///< DCMI global interrupt
    RNG_IRQn                    = 80,     ///< RNG global Interrupt
    FPU_IRQn                    = 81      ///< FPU global interrupt
} IRQn_Type;

/**
 * \brief  GPIO_pins_define GPIO pins define (from stm32f4xx_hal_gpio.h)
 */
#define GPIO_PIN_0      ((uint16_t)0x0001)  ///< Pin 0 selected
#define GPIO_PIN_1      ((uint16_t)0x0002)  ///< Pin 1 selected
#define GPIO_PIN_2      ((uint16_t)0x0004)  ///< Pin 2 selected
#define GPIO_PIN_3      ((uint16_t)0x0008)  ///< Pin 3 selected
#define GPIO_PIN_4      ((uint16_t)0x0010)  ///< Pin 4 selected
#define GPIO_PIN_5      ((uint16_t)0x0020)  ///< Pin 5 selected
#define GPIO_PIN_6      ((uint16_t)0x0040)  ///< Pin 6 selected
#define GPIO_PIN_7      ((uint16_t)0x0080)  ///< Pin 7 selected
#define GPIO_PIN_8      ((uint16_t)0x0100)  ///< Pin 8 selected
#define GPIO_PIN_9      ((uint16_t)0x0200)  ///< Pin 9 selected
#define GPIO_PIN_10     ((uint16_t)0x0400)  ///< Pin 10 selected
#define GPIO_PIN_11     ((uint16_t)0x0800)  ///< Pin 11 selected
#define GPIO_PIN_12     ((uint16_t)0x1000)  ///< Pin 12 selected
#define GPIO_PIN_13     ((uint16_t)0x2000)  ///< Pin 13 selected
#define GPIO_PIN_14     ((uint16_t)0x4000)  ///< Pin 14 selected
#define GPIO_PIN_15     ((uint16_t)0x8000)  ///< Pin 15 selected

/**
 * \brief   Peripheral memory map
 */
#define PERIPH_BASE         0x40000000UL    ///< Peripheral base address in the alias region
#define APB1PERIPH_BASE     PERIPH_BASE
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE     (PERIPH_BASE + 0x10000000UL)

/**
 * \brief   AHB1 peripherals
 */
#define GPIOA_BASE      (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE      (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE      (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE      (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE      (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOF_BASE      (AHB1PERIPH_BASE + 0x1400UL)
#define GPIOG_BASE      (AHB1PERIPH_BASE + 0x1800UL)
#define GPIOH_BASE      (AHB1PERIPH_BASE + 0x1C00UL)
#define GPIOI_BASE      (AHB1PERIPH_BASE + 0x2000UL)

/**
 * \brief   Peripheral_declaration
 */
#define GPIOA           ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB           ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC           ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD           ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE           ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOF           ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG           ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH           ((GPIO_TypeDef *) GPIOH_BASE)
#define GPIOI           ((GPIO_TypeDef *) GPIOI_BASE)


void __NOP(void);

void HAL_Delay(uint32_t Delay);

#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */


/**
 * \brief   Cortex-M NVIC control surface (fake). Bodies in stm32f4xx_hal.c
 *          are no-ops: on real hardware the drivers call these purely for
 *          their interrupt-controller side effects, which the native unit
 *          tests neither configure nor observe. Shared by every driver that
 *          installs an IRQ (DMA, SPI, I2C, USART, ...).
 */
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);
void HAL_NVIC_ClearPendingIRQ(IRQn_Type IRQn);


/**
 * \brief   HAL status enumeration -- minimal subset, added for the RNG fake
 *          surface needed by the L12 multi-thread Rng test. Matches the real
 *          HAL ordering so HAL_OK is the canonical success token.
 */
typedef enum
{
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

/**
 * \brief   Reset and Clock Control -- minimal subset (only the fields
 *          Rng::IsRngClockConfigured() reads). Backed by storage in the
 *          companion fake .cpp; the test harness pre-fills CR/PLLCFGR so
 *          the clock check passes.
 */
typedef struct
{
    volatile uint32_t CR;        ///< Clock control register
    volatile uint32_t PLLCFGR;   ///< PLL configuration register
} RCC_TypeDef;

extern RCC_TypeDef* const RCC;

#define RCC_CR_PLLRDY            ((uint32_t)0x02000000U)   ///< CR bit 25: main PLL ready flag
#define RCC_PLLCFGR_PLLQ         ((uint32_t)0x0F000000U)   ///< PLLCFGR PLLQ mask (bits 24..27)
#define RCC_PLLCFGR_PLLQ_Pos     ((uint32_t)24U)           ///< PLLCFGR PLLQ bit position

/**
 * \brief   RNG peripheral instance (opaque -- the fake never inspects it;
 *          only Rng.cpp's `mHandle.Instance = RNG;` touches it).
 */
typedef struct
{
    uint32_t reserved;
} RNG_TypeDef;

extern RNG_TypeDef* const RNG;

typedef struct
{
    RNG_TypeDef* Instance;   ///< Set to RNG by the driver; unread by the fake.
} RNG_HandleTypeDef;

#define __HAL_RCC_RNG_CLK_ENABLE()    do { } while(0)
#define __HAL_RCC_RNG_CLK_DISABLE()   do { } while(0)

/** Driver-facing HAL surface (fake implementations live in stm32f4xx_hal_rng.cpp). */
HAL_StatusTypeDef HAL_RNG_Init(RNG_HandleTypeDef* hrng);
HAL_StatusTypeDef HAL_RNG_DeInit(RNG_HandleTypeDef* hrng);
HAL_StatusTypeDef HAL_RNG_GenerateRandomNumber(RNG_HandleTypeDef* hrng, uint32_t* random32bit);

/**
 * \brief   Test-only observation hooks for the L12 multi-thread Rng test.
 * \details The fake's HAL_RNG_GenerateRandomNumber increments an atomic
 *          counter on entry and decrements on exit, tracking the maximum
 *          observed concurrent callers. The production atomic_flag guard
 *          in Rng::GetRandom must keep this <= 1; the test asserts it.
 */
int  FakeRNG_MaxObservedConcurrentCallers(void);
void FakeRNG_ResetObservation(void);


/**
 * \brief   APB peripheral-bus clock frequencies (fake). Fixed to the values
 *          the drafted 168 MHz PLL config produces: PCLK1 (APB1) = 42 MHz,
 *          PCLK2 (APB2) = 84 MHz. SPI1 sits on APB2, SPI2/3 on APB1, so the
 *          SPI driver reads these to validate the requested bus speed and to
 *          compute the baud-rate prescaler. Bodies in stm32f4xx_hal.c.
 */
uint32_t HAL_RCC_GetPCLK1Freq(void);
uint32_t HAL_RCC_GetPCLK2Freq(void);


/**
 * \brief   SPI peripheral instance (opaque). The driver only stores it in
 *          mHandle.Instance and compares handle->Instance == SPIx in its ISR
 *          dispatch; the fake never inspects the contents.
 */
typedef struct
{
    uint32_t reserved;
} SPI_TypeDef;

extern SPI_TypeDef* const SPI1;
extern SPI_TypeDef* const SPI2;
extern SPI_TypeDef* const SPI3;

/**
 * \brief   SPI init config -- field names mirror the real HAL so SPI::Init
 *          populates the handle exactly as on hardware.
 */
typedef struct
{
    uint32_t Mode;                ///< SPI_MODE_*
    uint32_t Direction;           ///< SPI_DIRECTION_*
    uint32_t DataSize;            ///< SPI_DATASIZE_*
    uint32_t CLKPolarity;         ///< SPI_POLARITY_*
    uint32_t CLKPhase;            ///< SPI_PHASE_*
    uint32_t NSS;                 ///< SPI_NSS_*
    uint32_t BaudRatePrescaler;   ///< SPI_BAUDRATEPRESCALER_*
    uint32_t FirstBit;            ///< SPI_FIRSTBIT_*
    uint32_t TIMode;              ///< SPI_TIMODE_*
    uint32_t CRCCalculation;      ///< SPI_CRCCALCULATION_*
    uint32_t CRCPolynomial;       ///< CRC polynomial
} SPI_InitTypeDef;

/* Forward declaration: the DMA handle is fully defined in stm32f4xx_hal_dma.h.
   SPI_HandleTypeDef only needs pointers to it (the hdmatx/hdmarx slots wired
   by __HAL_LINKDMA), so an incomplete type is sufficient here and avoids a
   hard include dependency for the many TUs that pull in this umbrella header
   without ever touching DMA. */
struct __DMA_HandleTypeDef;

/**
 * \brief   SPI handle. hdmatx/hdmarx are populated by __HAL_LINKDMA when a
 *          DMA stream is wired into the Tx/Rx slot; the WriteDMA/ReadDMA paths
 *          guard on them being non-null.
 */
typedef struct __SPI_HandleTypeDef
{
    SPI_TypeDef*                Instance;   ///< Set to SPIx by the driver
    SPI_InitTypeDef             Init;       ///< Configuration
    struct __DMA_HandleTypeDef* hdmatx;     ///< Tx DMA slot (set by __HAL_LINKDMA)
    struct __DMA_HandleTypeDef* hdmarx;     ///< Rx DMA slot (set by __HAL_LINKDMA)
} SPI_HandleTypeDef;


/**
 * \brief   Cortex-M4 DWT cycle counter and CoreDebug DEMCR (fake). Drivers that
 *          busy-wait on the cycle counter (e.g. I2C bus-recovery's
 *          DelayMicroseconds) read these; storage lives in stm32f4xx_hal.c.
 */
typedef struct
{
    volatile uint32_t CTRL;     ///< Control register (CYCCNTENA in bit 0)
    volatile uint32_t reserved[5];
    volatile uint32_t CYCCNT;   ///< Cycle count register
} DWT_TypeDef;

typedef struct
{
    volatile uint32_t DEMCR;    ///< Debug Exception and Monitor Control (TRCENA in bit 24)
} CoreDebug_TypeDef;

extern DWT_TypeDef*       const DWT;
extern CoreDebug_TypeDef* const CoreDebug;

#define DWT_CTRL_CYCCNTENA_Msk     ((uint32_t)0x00000001U)   ///< CTRL: CYCCNT enable
#define CoreDebug_DEMCR_TRCENA_Msk ((uint32_t)0x01000000U)   ///< DEMCR: trace enable

/**
 * \brief   Live core clock (fake). Deliberately 0 so the cycle budget
 *          `(SystemCoreClock / 1000000) * us` computes to 0 and every
 *          DWT-based busy-wait returns immediately: the fake DWT->CYCCNT does
 *          not advance on read, so a non-zero budget would spin forever. Native
 *          tests have no real bus, so a zero-length delay is the correct fake.
 */
extern uint32_t SystemCoreClock;


/**
 * \brief   I2C peripheral instance. Only CR1 is modelled (the bus-recovery
 *          SWRST / PE-disable path writes it); the remaining registers are
 *          omitted as no driver path inspects them.
 */
typedef struct
{
    volatile uint32_t CR1;   ///< Control register 1 (PE, SWRST)
} I2C_TypeDef;

extern I2C_TypeDef* const I2C1;
extern I2C_TypeDef* const I2C2;
extern I2C_TypeDef* const I2C3;

/**
 * \brief   I2C init config -- field names mirror the real HAL so I2C::Init
 *          populates the handle exactly as on hardware.
 */
typedef struct
{
    uint32_t ClockSpeed;        ///< Bus clock in Hz
    uint32_t DutyCycle;         ///< I2C_DUTYCYCLE_*
    uint32_t OwnAddress1;       ///< First device own address
    uint32_t AddressingMode;    ///< I2C_ADDRESSINGMODE_*
    uint32_t DualAddressMode;   ///< I2C_DUALADDRESS_*
    uint32_t OwnAddress2;       ///< Second device own address
    uint32_t GeneralCallMode;   ///< I2C_GENERALCALL_*
    uint32_t NoStretchMode;     ///< I2C_NOSTRETCH_*
} I2C_InitTypeDef;

/**
 * \brief   I2C handle. hdmatx/hdmarx are populated by __HAL_LINKDMA; Devaddress
 *          and ErrorCode are read by the Sleep abort path / completion ISRs.
 */
typedef struct __I2C_HandleTypeDef
{
    I2C_TypeDef*                Instance;     ///< Set to I2Cx by the driver
    I2C_InitTypeDef             Init;         ///< Configuration
    struct __DMA_HandleTypeDef* hdmatx;       ///< Tx DMA slot (set by __HAL_LINKDMA)
    struct __DMA_HandleTypeDef* hdmarx;       ///< Rx DMA slot (set by __HAL_LINKDMA)
    uint16_t                    Devaddress;   ///< Target address of an in-flight transfer
    uint32_t                    ErrorCode;    ///< HAL_I2C_ERROR_* of the last transfer
} I2C_HandleTypeDef;


/**
 * \brief   USART peripheral instance. SR is read by the IDLE-flag check and DR
 *          by the Rx-complete dummy read; the remaining registers are omitted.
 */
typedef struct
{
    volatile uint32_t SR;   ///< Status register (IDLE flag)
    volatile uint32_t DR;   ///< Data register
} USART_TypeDef;

extern USART_TypeDef* const USART1;
extern USART_TypeDef* const USART2;
extern USART_TypeDef* const USART3;
extern USART_TypeDef* const USART6;

/**
 * \brief   UART init config -- field names mirror the real HAL so USART::Init
 *          populates the handle exactly as on hardware.
 */
typedef struct
{
    uint32_t BaudRate;      ///< Bus baud rate
    uint32_t WordLength;    ///< UART_WORDLENGTH_*
    uint32_t StopBits;      ///< UART_STOPBITS_*
    uint32_t Parity;        ///< UART_PARITY_*
    uint32_t Mode;          ///< UART_MODE_*
    uint32_t HwFlowCtl;     ///< UART_HWCONTROL_*
    uint32_t OverSampling;  ///< UART_OVERSAMPLING_*
} UART_InitTypeDef;

/**
 * \brief   UART handle. hdmatx/hdmarx are populated by __HAL_LINKDMA;
 *          RxXferSize/RxXferCount are read by the Rx-complete ISR to compute
 *          the received byte count.
 */
typedef struct __UART_HandleTypeDef
{
    USART_TypeDef*              Instance;     ///< Set to USARTx by the driver
    UART_InitTypeDef            Init;         ///< Configuration
    struct __DMA_HandleTypeDef* hdmatx;       ///< Tx DMA slot (set by __HAL_LINKDMA)
    struct __DMA_HandleTypeDef* hdmarx;       ///< Rx DMA slot (set by __HAL_LINKDMA)
    uint16_t                    RxXferSize;   ///< Bytes expected in the active Rx
    uint16_t                    RxXferCount;  ///< Bytes still outstanding in the active Rx
} UART_HandleTypeDef;


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_H
