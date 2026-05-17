/**
 * \file    heap_check.h
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Heap check functions for ST Cortex-M4.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/drivers/utility/HeapCheck
 *
 * \author  Terry Louwers (terry.louwers@fourtress.nl)
 * \version 1.1
 * \date    11-2019
 */

#ifndef HEAP_CHECK_H_
#define HEAP_CHECK_H_

#ifdef __cplusplus
extern "C" {
#endif


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <stdint.h>
#include <stdbool.h>


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
/**
 * \brief   Magic value the user's _sbrk() must write at __sbrk_heap_end on
 *          every real allocation (incr > 0). end_of_heap_overrun() reads
 *          this word back; if a stack-into-heap overrun has corrupted it,
 *          the function reports true.
 * \note    Single source of truth: both heap_check.c and the user's
 *          _sbrk() reference this macro so the two cannot drift. See
 *          README.md "Modification" for the required _sbrk shape.
 */
#define HEAP_END_MARKER  0xFAFBFCFDU


/************************************************************************/
/* Functions                                                            */
/************************************************************************/
uint32_t get_total_heap(void);
uint32_t get_used_heap(void);
uint32_t* get_start_of_heap(void);
bool end_of_heap_overrun(void);


#ifdef __cplusplus
}
#endif


#endif  // HEAP_CHECK_H_
