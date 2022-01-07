/**
 * \file    heap_check.c
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \brief   Heap check functions for ST Cortex-M4.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/utility/HeapCheck
 *
 * \author  Terry Louwers (terry.louwers@fourtress.nl)
 * \version 1.0
 * \date    11-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "heap_check.h"
#include <stddef.h>         // ptrdiff_t


/************************************************************************/
/* Externals                                                            */
/************************************************************************/
/**
 * \brief   Use "end" as provided by linker. It points to the first
 *          address after the top of the .bss segment and marks the start of
 *          the heap.
 */
extern uint8_t end asm("end");

/**
 * \brief   Use heap size as specified in the linked control script.
 */
extern uint32_t _Min_Heap_Size;

/**
 * \brief   External declaration for _sbrk, in sysmem.c.
 * \param   incr    Size to increment with.
 * \return  Pointer to current used heap memory.
 */
extern void *_sbrk(ptrdiff_t incr);


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
/**
 * \brief   A 'magic' marker indicating the end of the heap. Must be equal
 *          to the one used in _sbrk.
 */
const uint32_t HEAP_END_MARKER = 0xFAFBFCFD;


/************************************************************************/
/* Public functions                                                     */
/************************************************************************/
/**
 * \brief   Get the total heap memory size.
 * \return  Total heap size in bytes.
 */
uint32_t get_total_heap(void)
{
    return (uint32_t)&_Min_Heap_Size;
}

/**
 * \brief   Get the used heap memory size.
 * \return  Used heap size in bytes.
 */
uint32_t get_used_heap(void)
{
    uint8_t* heap_end   = (uint8_t*)_sbrk(0);
    uint8_t* heap_start = (uint8_t*)&end;

    return (heap_end - heap_start);
}

/**
 * \brief   Get a pointer to the start of the heap.
 * \return  Start of the heap as address.
 */
uint32_t* get_start_of_heap(void)
{
    return (uint32_t*)&end;
}

/**
 * \brief   Check if the heap has been overwritten by the stack.
 * \note    It could be we never reach here as overwriting the heap could
 *          cause HardFaults as well.
 * \returns True if the heap is overwritten by the stack, else false.
 */
bool end_of_heap_overrun(void)
{
    uint8_t* heap_end = (uint8_t*)_sbrk(0);

    if (heap_end == (uint8_t*)-1)
    {
        return true;
    }

    // Note: the casting is to prevent compiler warnings.
    return ( *((uint32_t*)((void*)heap_end)) != HEAP_END_MARKER );
}
