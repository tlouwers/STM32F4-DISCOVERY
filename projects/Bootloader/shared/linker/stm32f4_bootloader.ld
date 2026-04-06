/*
 * STM32F4 Bootloader Linker Script
 *
 * Layout (STM32F407VG, 1 MiB flash assumed):
 *  - Bootloader:  0x08000000 — 0x0800FFFF  (64K)
 *  - Metadata area:0x08010000 — 0x0801FFFF  (64K)
 *  - Application:  0x08020000 — 0x080FFFFF  (remaining flash ~896K)
 */

MEMORY
{
    CCMRAM    (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
    FLASH_BL  (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    FLASH_META(r)   : ORIGIN = 0x08010000, LENGTH = 64K
    RAM       (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Default entry point provided by the startup code */
ENTRY(Reset_Handler)

    /* Vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH_BL

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.rodata)
        *(.rodata*)

        KEEP (*(.init))
        KEEP (*(.fini))

        . = ALIGN(4);
        PROVIDE(__etext = .);
    } > FLASH_BL

    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH_BL

    .ARM :
    {
        . = ALIGN(4);
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH_BL

    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);
    } > FLASH_BL

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);
    } > FLASH_BL

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
        . = ALIGN(4);
    } > FLASH_BL

    /* Used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data sections into RAM, load in FLASH_BL */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH_BL

    /* CCM-RAM section */
    _siccmram = LOADADDR(.ccmram);
    .ccmram : AT> FLASH_BL
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } > CCMRAM

    /* Uninitialized data */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    /* Metadata region reserved in flash (kept empty by linker) */
    .metadata (NOLOAD) :
    {
        KEEP(*(.metadata))
    } > FLASH_META

    /* User heap and stack */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* Remove unused library sections */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    /* Provide runtime symbols */
    PROVIDE(_etext = __etext);
    PROVIDE(_sdata = _sdata);
    PROVIDE(_edata = _edata);
    PROVIDE(_sbss  = _sbss);
    PROVIDE(_ebss  = _ebss);
    /* Provide symbols for runtime */
    PROVIDE(_etext = __etext);
    PROVIDE(_sdata = __data_start__);
    PROVIDE(_edata = __data_end__);
    PROVIDE(_sbss  = __bss_start__);
    PROVIDE(_ebss  = __bss_end__);

    /* Heap/stack placement (symbolic only) */
    ._heap (NOLOAD) : { . = ALIGN(8); } > RAM
}

/* End of script */
