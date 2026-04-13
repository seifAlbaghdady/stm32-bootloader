/*
 * Linker script — STM32F411RE Bootloader
 * Flash: 512 KB at 0x08000000
 * RAM:   128 KB at 0x20000000
 *
 * Bootloader occupies sectors 0–2 (48 KB total).
 * Application starts at sector 3: 0x0800C000.
 */

MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 48K   /* sectors 0-2 */
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

ENTRY(Reset_Handler)

SECTIONS {

    /* ── Interrupt vector table (must be at flash origin) ── */
    .isr_vector : {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ── Code and read-only data ──────────────────────────── */
    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ── Initialized data — stored in flash, copied to RAM ── */
    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH
    _sidata = LOADADDR(.data);

    /* ── Zero-initialized data (BSS) ─────────────────────── */
    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* ── Stack ───────────────────────────────────────────── */
    .stack : {
        . = ALIGN(8);
        . += 0x800;    /* 2 KB stack for bootloader */
        _estack = .;
    } > RAM
}
