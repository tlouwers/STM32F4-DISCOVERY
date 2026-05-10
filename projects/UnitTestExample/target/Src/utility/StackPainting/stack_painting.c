/**
 * \file    stack_painting.c
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
 * \author  Terry Louwers (terry.louwers@fourtress.nl)
 * \version 1.1
 * \date    11-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "stack_painting.h"
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Externals                                                            */
/************************************************************************/
/**
 * \brief   Use top of stack as specified in the linker control script.
 */
extern uint32_t _estack;

/**
 * \brief   Use the linker-reserved stack size. The address of this symbol
 *          encodes the size in bytes (linker-symbol-as-size pattern, same
 *          as _Min_Heap_Size in HeapCheck).
 */
extern uint32_t _Min_Stack_Size;


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
/**
 * \brief   A 'magic' number to 'paint' the stack. Although theoretically
 *          possible a stack value is the same as the number, it is
 *          very unlikely and not repeated for long.
 */
static const uint32_t PAINT_VALUE = 0xC5C5C5C5U;


/************************************************************************/
/* Public functions                                                     */
/************************************************************************/
/**
 * \brief   Fills all currently-unused stack with PAINT_VALUE so that
 *          get_used_stack() can later detect the high-water mark.
 * \note    Should be done as one of the first things in main(). The painted
 *          range is exactly [_estack - _Min_Stack_Size, current MSP); the
 *          live frame at and above MSP is left untouched, and the heap
 *          region (which sits below the stack on this F407 layout) is
 *          NOT touched.
 */
void paint_stack(void)
{
    // Top of the painted area is the current MSP -- bytes at and above MSP
    // are the live stack frame (us and our caller) and must not be
    // overwritten.
    const uint32_t msp = __get_MSP();

    // Bottom of the actual stack region is _estack - _Min_Stack_Size, NOT
    // _ebss: the heap lives between _ebss and the stack and must not be
    // painted (would corrupt anything malloc'd by pre-main static-object
    // ctors, and on a clean boot is just wasted cycles).
    const uint32_t stack_bottom = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;

    uint32_t* p = (uint32_t*)stack_bottom;
    const uint32_t words_to_paint = (msp - stack_bottom) / 4;

    for (uint32_t i = 0; i < words_to_paint; i++)
    {
        *p++ = PAINT_VALUE;
    }
}

/**
 * \brief   Get the total amount of stack reserved by the linker.
 * \return  Total stack size in bytes.
 */
uint32_t get_total_stack(void)
{
    return (uint32_t)&_Min_Stack_Size;
}

/**
 * \brief   Get the high-water-mark stack usage.
 * \details Walks the stack downward from the current MSP looking for the
 *          first surviving PAINT_VALUE; the offset of that word from
 *          _estack is the deepest the stack has ever grown.
 * \note    If the entire painted region has been overwritten (i.e., the
 *          stack overflowed into the gap below it at some point) the
 *          loop runs to completion without finding a paint marker and the
 *          returned value will be the full reserved stack size or larger.
 *          Treat that as "stack overflow occurred" rather than a literal
 *          high-water mark.
 * \return  Used stack size in bytes (high-water mark since paint_stack()).
 */
uint32_t get_used_stack(void)
{
    // Prevent interrupts during this section so an ISR's transient pushes
    // below MSP don't show up as "used stack".
    const uint32_t primask_state = __get_PRIMASK();
    __disable_irq();

    uint32_t* sp = (uint32_t*)__get_MSP();

    // Bottom of the actual stack region (see paint_stack() for rationale).
    const uint32_t* stack_bottom = (const uint32_t*)((uint32_t)&_estack - (uint32_t)&_Min_Stack_Size);
    const uint32_t words_to_search = sp - stack_bottom;

    // Search from current SP downward, stop at the first surviving paint.
    for (uint32_t i = 0; i < words_to_search; i++)
    {
        if (*sp == PAINT_VALUE)
        {
            break;
        }
        sp--;
    }

    // Restore interrupts to whatever they were before.
    __set_PRIMASK(primask_state);

    // sp now points at the first painted word; the highest used word is
    // one above it, so subtract sizeof(uint32_t) to get the byte offset
    // from _estack of the deepest stack-touched word.
    return (uint32_t)&_estack - (uint32_t)sp - 4;
}
