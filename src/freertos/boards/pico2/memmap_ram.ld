/*
 * SPDX-License-Identifier: Apache-2.0
 * RAM-only linker script for RP2350 debugging
 * Load and execute entirely from SRAM
 */

MEMORY
{
    RAM(rwx) : ORIGIN = 0x20000000, LENGTH = 512K
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table at start of RAM */
    .vectors :
    {
        . = ALIGN(256);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > RAM

    /* Code section */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > RAM

    /* ARM exception handling */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > RAM

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > RAM

    /* Data section - already in RAM, no copy needed */
    _sidata = .;
    
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

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

    /* Heap */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __end__ = .;
        PROVIDE(end = .);
        PROVIDE(__HeapBase = .);
        . = . + 0x8000;  /* 32KB heap */
        PROVIDE(__HeapLimit = .);
    } > RAM

    /* Stack at end */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + 0x2000;   /* 8KB stack */
        __StackTop = .;
        _estack = .;
    } > RAM

    PROVIDE(__stack = __StackTop);
}
