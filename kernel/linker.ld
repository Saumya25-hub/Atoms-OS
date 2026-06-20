OUTPUT_FORMAT("binary")
ENTRY(_start)

SECTIONS
{
    /* The kernel is loaded at 1MB (0x100000) */
    . = 0x100000;

    .text ALIGN(4096) :
    {
        build/kernel_entry.o(.text*)
        *(.text*)
    }

    .rodata ALIGN(4096) :
    {
        *(.rodata*)
    }

    .data ALIGN(4096) :
    {
        *(.data*)
    }

    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss*)
    }

    _kernel_end = .;
}
