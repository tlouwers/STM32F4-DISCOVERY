/**
 * \file    stack_painting.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Stack painting functions for ST Cortex-M4.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/utility/StackPainting
 *
 * \note    paint_stack() should be called as early as possible in main()
 *          so the recorded high-water mark covers as much of the program's
 *          execution as possible. Anything that ran (and used stack)
 *          before paint_stack() is invisible to get_used_stack().
 *
 * \note    Only the MSP stack is tracked. Under FreeRTOS the per-task
 *          stacks live on PSP and have their own paint/highwater-mark
 *          mechanism (uxTaskGetStackHighWaterMark); this utility measures
 *          only the main/ISR stack.
 *
 * \author  Terry Louwers (terry.louwers@fourtress.nl)
 * \version 1.1
 * \date    11-2019
 */

#ifndef STACK_PAINTING_H_
#define STACK_PAINTING_H_

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>


/************************************************************************/
/* Functions                                                            */
/************************************************************************/
void paint_stack(void);

uint32_t get_total_stack(void);
uint32_t get_used_stack(void);


#ifdef __cplusplus
}
#endif


#endif  // STACK_PAINTING_H_
