/**
 * \file stack_painting.c
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Stack painting functions for ST Cortex-M4.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/utility/StackPainting
 *
 * \details This code is intended to be used to determine the stack usage at
 *          runtime. The code is implemented in 'C', to be usable in both 'C'
 *          and 'C++' projects.
 */
#ifdef DOXYGEN_SHOULD_SKIP_THIS
/*          Example:
 *          // Include the header file
 *          #include "stack_painting.h"
 *
 *          // Right after the start of main(), 'paint' the stack:
 *          void main(void)
 *          {
 *              paint_stack();          <-- here
 *
 *              // Clocks, pins, remainder ...
 *          }
 *
 *          // At certain intervals log the used stack memory (not too often, every 10 seconds or so):
 *          static volatile uint32_t used_stack = 0;			<-- global to store the (growing) stack value
 *          void Application::GetUsedStack()
 *          {
 *              uint32_t tmp = get_used_stack();
 *              if (tmp > used_stack )
 *              {
 *                  used_stack = tmp;
 *              }
 *          }
 *
 *          // To get an idea of the total stack available:
 *          void Application::GetTotalStack()
 *          {
 *              uint32_t total_stack = get_total_stack();
 *          }
 */
#endif
/*
 * \note    This code is not to be used 'as-is': be sure you know where the
 *          stack and heap are located in your project and modify the code to
 *          match these areas.
 *
 * \note    Inspiration from:
 *          https://ucexperiment.wordpress.com/2015/01/02/arduino-stack-painting/
 *          https://embeddedgurus.com/stack-overflow/2009/03/computing-your-stack-size/
 *
 * \author  Terry Louwers (terry.louwers@fourtress.nl)
 * \version 1.0
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
 * \brief   Use bottom of the stack: end of the bss section as specified in
 *          the linker control script. Note that this is the start of the
 *          heap (which may or may not be used yet).
 */
extern uint32_t _ebss;


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
/**
 * \brief   A 'magic' number to 'paint' the stack. Although theoretically
 *          possible a stack value is the same as the number, it is
 *          very unlikely and not repeated for long.
 */
const uint32_t PAINT_VALUE = 0xC5C5C5C5;


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
static uint32_t total_stack_size = 0;
static uint32_t used_stack_size  = 0;


/************************************************************************/
/* Public functions                                                     */
/************************************************************************/
/**
 * \brief   Fills all of the stack with a defined value (PAINT_VALUE).
 * \note    Should be done as one of the first things in main().
 */
void paint_stack(void)
{
    // Top of the stack is the _estack address. The size occupied by the application
    // is to be subtracted, this we can retrieve by requesting the current stack pointer.
    const uint32_t application = __get_MSP();

    // Bottom of the stack is the end of the bss section (also: the start of the heap).
    uint32_t* bottom_of_stack = (uint32_t*)&_ebss;

    // Find out what needs to be 'painted' - in sizeof(uint32_t)
    uint32_t area_to_paint = (application - (uint32_t)bottom_of_stack) / 4;

    // Paint that area
    for (uint32_t i = 0; i < area_to_paint; i++)
    {
        *bottom_of_stack++ = PAINT_VALUE;
    }
}

/**
 * \brief   Get the total amount of stack available.
 * \return  Total stack size in bytes.
 */
uint32_t get_total_stack(void)
{
    // Could be we never called 'get_used_stack' before.
    if (total_stack_size == 0)
    {
        get_used_stack();
    }

    return total_stack_size;
}

/**
 * \brief   Get the (once) used stack size.
 * \return  Used stack size in bytes.
 */
uint32_t get_used_stack(void)
{
    // Prevent interrupts during this section
    uint32_t primask_state = __get_PRIMASK();
    __disable_irq();

    // Instead of the top of the stack, use the start from the current stack pointer.
    uint32_t* application = (uint32_t*)__get_MSP();

    // Bottom of the stack is the end of the bss section (also: the start of the heap).
    const uint32_t* bottom_of_stack = (uint32_t*)&_ebss;

    // Find out what needs to be searched - in sizeof(uint32_t)
    uint32_t area_to_search = (application - bottom_of_stack);

    // Search from top (current stack pointer) to bottom, upto the bss section.
    for (uint32_t i = 0; i < area_to_search; i++)
    {
        if (*application == PAINT_VALUE)
        {
            break;
        }
        application--;
    }

    // Restore interrupts
    if (!primask_state) { __enable_irq(); }

    total_stack_size = (area_to_search * 4);                            // * 4: uint32_t to byte
    used_stack_size  = (uint32_t)&_estack - (uint32_t)application - 4;  // - 4: stopped on still painted value, top of the stack is the _estack address.

    return used_stack_size;
}
