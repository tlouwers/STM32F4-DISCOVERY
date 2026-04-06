#ifndef __STM32F4xx_HAL_H
#define __STM32F4xx_HAL_H


#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

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
 * \brief   Peripheral memory map
 */
#define PERIPH_BASE         0x40000000UL
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

#define GPIOA           ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB           ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC           ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD           ((GPIO_TypeDef *) GPIOD_BASE)


void __NOP(void);

HAL_StatusTypeDef HAL_Init(void);
void HAL_Delay(uint32_t Delay);

#define UNUSED(X) (void)X


#ifdef __cplusplus
}
#endif


#endif  // __STM32F4xx_HAL_H
