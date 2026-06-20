; ==============================================================================
; SignaturesOS - Memory Map Constants
; ==============================================================================

; Physical Memory Map
BOOT_ADDR           equ 0x7C00      ; Stage 1 Bootloader load address (BIOS default)
STAGE2_ADDR         equ 0x8000      ; Stage 2 Bootloader load address
PAGE_TABLE_BASE     equ 0x10000     ; 4-Level Page Tables (PML4 starts here)
KERNEL_BUFFER       equ 0x20000     ; Temporary buffer for raw kernel disk read
STACK_TOP           equ 0x90000     ; 32-bit and 64-bit stack top (grows downwards)
KERNEL_EXEC         equ 0x100000    ; Final kernel execution address (1MB mark)

; Hardware Mapped Memory
VGA_MEMORY          equ 0xB8000     ; VGA Text Mode buffer base address
