/**
  ******************************************************************************
  * @file    stm32f4xx_hal_timebase_TIM.c
  * @brief   HAL time base based on the hardware TIM.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  * @note This file is intended as a 'drop-in' functionality to provide HW
  *       timer 14 as HAL clock tick. The timer is configured to run at
  *       1000 Hz. It needs an implementation of function:
  *       'void TIM8_TRG_COM_TIM14_IRQHandler(void);' in file:
  *       'stm32f4xx_it.c'.
  *
  ******************************************************************************
  */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"


/************************************************************************/
/* Variables                                                            */
/************************************************************************/
/**
 * \brief   Timer 14 variable, used as HAL timer ticks provider.
 */
TIM_HandleTypeDef        htim14;


/************************************************************************/
/* Functions                                                            */
/************************************************************************/
/**
  * @brief  This function configures the TIM14 as a time base source.
  *         The time source is configured  to have 1ms time base with a dedicated
  *         Tick interrupt priority.
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
  * @param  TickPriority  Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef  clkconfig;
    uint32_t            uwTimclock = 0;
    uint32_t            uwPrescalerValue = 0;
    uint32_t            pFLatency;

    /*Configure the TIM14 IRQ priority */
    HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, TickPriority ,0);

    /* Enable the TIM14 global Interrupt */
    HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);

    /* Enable TIM14 clock */
    __HAL_RCC_TIM14_CLK_ENABLE();

    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    /* Compute TIM14 clock */
    uwTimclock = HAL_RCC_GetPCLK1Freq();

    /* Compute the prescaler value to have TIM14 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

    /* Initialize TIM14 */
    htim14.Instance = TIM14;

    /* Initialize TIMx peripheral as follows:
       + Period = [(TIM14CLK/1000) - 1]. to have a (1/1000) s time base.
       + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
       + ClockDivision = 0
       + Counter direction = Up
    */
    htim14.Init.Period = (1000000U / 1000U) - 1U;
    htim14.Init.Prescaler = uwPrescalerValue;
    htim14.Init.ClockDivision = 0;
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    if(HAL_TIM_Base_Init(&htim14) == HAL_OK)
    {
        /* Start the TIM time Base generation in interrupt mode */
        return HAL_TIM_Base_Start_IT(&htim14);
    }

    /* Return function status */
    return HAL_ERROR;
}

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM14 update interrupt.
  */
void HAL_SuspendTick(void)
{
    /* Disable TIM14 update Interrupt */
    __HAL_TIM_DISABLE_IT(&htim14, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM14 update interrupt.
  */
void HAL_ResumeTick(void)
{
    /* Enable TIM14 Update interrupt */
    __HAL_TIM_ENABLE_IT(&htim14, TIM_IT_UPDATE);
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM14 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM14) {
        HAL_IncTick();
    }
}
