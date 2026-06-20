OUTPUT_FORMAT("binary")
ENTRY(kernel_main)

SECTIONS
{
    /* 
     * Kernel is loaded at 1MB mark (0x100000) by the bootloader.
     * This tells the linker to calculate all memory addresses relative to this point.
     */
    . = 0x100000;

    .text : ALIGN(4096)
    {
        *(.text)
        *(.rodata)
    }

    .data : ALIGN(4096)
    {
        *(.data)
    }

    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }
}
