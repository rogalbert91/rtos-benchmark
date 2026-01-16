/*
 * SPDX-License-Identifier: Apache-2.0
 * Linker script for RP2350 (Raspberry Pi Pico 2)
 * 
 * Memory Map:
 *   Flash: 4MB external QSPI starting at 0x10000000
 *   SRAM:  520KB starting at 0x20000000
 */

MEMORY
{
    FLASH(rx)  : ORIGIN = 0x10000000, LENGTH = 4096K
    RAM(rwx)   : ORIGIN = 0x20000000, LENGTH = 520K
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* Vector table must be at the start of flash */
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

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
    } > FLASH

    /* ARM exception handling */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Initialized data - copied from flash to RAM at startup */
    _sidata = LOADADDR(.data);
    
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* Uninitialized data - zeroed at startup */
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
        . = . + 0x10000;  /* 64KB heap */
        PROVIDE(__HeapLimit = .);
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + 0x2000;   /* 8KB stack */
        __StackTop = .;
        _estack = .;
    } > RAM

    PROVIDE(__stack = __StackTop);
}
