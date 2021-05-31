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


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_H
