/**
 * \file    TestHeapCheck.cpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Native unit tests for the HeapCheck utility
 *          (drivers/utility/HeapCheck/heap_check.c).
 *
 * \details heap_check.c imports three symbols the firmware gets from the linker
 *          script / sysmem.c: `end` (heap base), `_Min_Heap_Size` (its ADDRESS
 *          is the total heap size) and `_sbrk` (the program break). This test
 *          TU stands in for all three with a controlled heap region and a fake
 *          `_sbrk`, so every public query can be driven to a known answer.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/tests
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    05-2026
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "gtest/gtest.h"

extern "C" {
#include "utility/HeapCheck/heap_check.h"
}

#include <cstddef>
#include <cstdint>


/************************************************************************/
/* Controlled stand-ins for the linker / sysmem symbols                 */
/************************************************************************/
extern "C" {

// The linker heap-start symbol. heap_check.c binds it via `asm("end")`, which
// forces the un-mangled name `end`; we reference that SAME symbol (a normal C
// definition would be mangled to `_end` on i686 and would not bind). It is a
// real, fixed address, so get_used_heap()'s `_sbrk(0) - &end` arithmetic is
// driven purely by where we point the fake break -- no dereference of &end.
extern uint8_t end asm("end");

// The linker min-heap-size symbol. get_total_heap() returns its ADDRESS, not
// its value, so only the address has to be stable.
uint32_t _Min_Heap_Size = 0;

} // extern "C"


namespace {


// A 4-byte cell the overrun check can dereference safely (the real `end` is not
// ours to write past). The fake break points here for the marker tests.
uint32_t s_marker_cell = 0;

// The fake program break reported by _sbrk(0); tests move it to model usage.
uint8_t* s_break = &end;


} // namespace


extern "C" {

// Controllable program break. heap_check.c only ever calls _sbrk(0).
void* _sbrk(ptrdiff_t /*incr*/)
{
    return s_break;
}

} // extern "C"


namespace {


/************************************************************************/
/* Fixture                                                              */
/************************************************************************/
class HeapCheck_Test : public ::testing::Test
{
protected:
    HeapCheck_Test()
    {
        s_break = &end;         // empty heap by default
    }
};


/************************************************************************/
/* Tests                                                                */
/************************************************************************/
TEST_F(HeapCheck_Test, GetTotalHeap_ReturnsAddressOfMinHeapSizeSymbol)
{
    EXPECT_EQ(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&_Min_Heap_Size)),
              get_total_heap());
}

TEST_F(HeapCheck_Test, GetStartOfHeap_ReturnsHeapBaseSymbol)
{
    EXPECT_EQ(reinterpret_cast<uint32_t*>(static_cast<void*>(&end)),
              get_start_of_heap());
}

TEST_F(HeapCheck_Test, GetUsedHeap_EmptyHeap_ReturnsZero)
{
    s_break = &end;
    EXPECT_EQ(0U, get_used_heap());
}

TEST_F(HeapCheck_Test, GetUsedHeap_BreakAdvanced_ReturnsBytesConsumed)
{
    s_break = &end + 40;
    EXPECT_EQ(40U, get_used_heap());
}

TEST_F(HeapCheck_Test, EndOfHeapOverrun_MarkerIntact_ReturnsFalse)
{
    s_marker_cell = HEAP_END_MARKER;
    s_break = reinterpret_cast<uint8_t*>(&s_marker_cell);

    EXPECT_FALSE(end_of_heap_overrun());
}

TEST_F(HeapCheck_Test, EndOfHeapOverrun_MarkerClobbered_ReturnsTrue)
{
    s_marker_cell = 0xDEADBEEFU;   // stack walked over it
    s_break = reinterpret_cast<uint8_t*>(&s_marker_cell);

    EXPECT_TRUE(end_of_heap_overrun());
}

TEST_F(HeapCheck_Test, EndOfHeapOverrun_SbrkFailure_ReturnsTrue)
{
    s_break = reinterpret_cast<uint8_t*>(static_cast<intptr_t>(-1));

    EXPECT_TRUE(end_of_heap_overrun());
}


} // namespace
