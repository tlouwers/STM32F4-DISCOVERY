# HeapCheck
Low level functions to determine heap usage during run time.

## Description
Assuming there is no memory manager in use, the heap size can be queried.
To get an idea of the heap (dynamic memory currently in use), a trick is used: a low level method called `_sbrk()` is called with size 0. Usually this function is used by `malloc()` to request memory from the heap, but when we request nothing (size 0) we get the current heap address. Together with the start of the heap (address) we can determine how much heap memory is used. If the amount of memory increases over time (constantly), this indicates there is either a memory leak or memory fragmentation. Assuming the programmer handles the heap correctly (request and release memory) there should be no memory leaks, only fragmentation.

## Requirements
- GCC compiler (untested on other compilers)
- STM32F407G-DISC1 board (untested on other microcontrollers)

## Check
This code is not to be used 'as-is': be sure you know where the stack and heap are located in your project and modify the code to match these areas. In my (tested) case it was in a file called `STM32F407VGTX_FLASH.ld`.
Inspiration from: <https://github.com/angrave/SystemProgramming/wiki/Memory,-Part-1:-Heap-Memory-Introduction> and <http://library.softwareverify.com/memory-fragmentation-your-worst-nightmare/>

## Intended use
This code is intended to be used on an ST Cortex-M4, but should be easily portable to other platforms.
The code is implemented in 'C', to be usable in both 'C' and 'C++' projects.

## Change
To allow stack overflow detection (stack growing/running over allocated heap memory) a flag is added in the function `_sbrk()` which allocated the heap memory (see 'Modification' below). Every time memory is allocated it flags the end of the claimed heap memory with a flag. Later on, when checking regularly, if the flag gets overwritten by the stack we can determine if a stack overflow occurred. It is an indication only, as there is a chance the stack has exactly the same value as the flag (very unlikely), or that the device enters a HardFault before we can check the condition.

## Example
```cpp
// Include the header file
#include "heap_check.h"

// At a later point check where the block of memory can be allocated:
static volatile uint32_t used_heap = 0;        <-- global to store the (growing) heap value
void Application::GetUsedHeap()
{
    uint32_t tmp = get_used_heap();
    if (tmp > used_heap)
    {
        used_heap = tmp;
    }
}

// To check for stack overflow we can call a dedicated function (should be done regularly):
void Application::CheckForStackOverflow()
{
    if (end_of_heap_overrun())
    {
        // Log, or take action ...
    }
}
```

## Modification
To use the 'end_of_heap_overrun()', a modification in the function '_sbrk()' needs to be made. For ST this is in the file 'sysmem.c', around line 79. A flag needs to be added to mark the end of the heap.
```cpp
// Modified '_sbrk()' function:
void *_sbrk(ptrdiff_t incr)
{
  extern uint8_t _end; /* Symbol defined in the linker script */
  extern uint8_t _estack; /* Symbol defined in the linker script */
  extern uint32_t _Min_Stack_Size; /* Symbol defined in the linker script */
  const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
  const uint8_t *max_heap = (uint8_t *)stack_limit;
  uint8_t *prev_heap_end;

  /* Initialize heap end at first call */
  if (NULL == __sbrk_heap_end)
  {
    __sbrk_heap_end = &_end;
  }

  /* Protect heap from growing into the reserved MSP stack */
  if (__sbrk_heap_end + incr > max_heap)
  {
    errno = ENOMEM;
    return (void *)-1;
  }

  prev_heap_end = __sbrk_heap_end;
  __sbrk_heap_end += incr;
  *((uint32_t*)((void*)__sbrk_heap_end)) = 0xFAFBFCFD;   // Mark end of heap to detect stack overflow

  return (void *)prev_heap_end;
}
```
