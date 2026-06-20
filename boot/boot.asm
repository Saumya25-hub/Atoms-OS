; ==============================================================================
; SignaturesOS - Stage 1 Bootloader
; ==============================================================================
[ORG 0x7C00]        ; BIOS loads us exactly at 0x7C00
[BITS 16]           ; We start in 16-bit Real Mode

%define STAGE2_SECTORS 4       ; Number of sectors to read for Stage 2
%define STAGE2_OFFSET  0x8000  ; Clean memory address for Stage 2

start:
    ; 1. Normalize Segment Registers and Setup Stack
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Save the boot drive number provided by BIOS in DL
    mov [BOOT_DRIVE], dl

    ; 2. Print "Stage1 OK" message (PM Recommendation)
    mov si, stage1_msg
print_s1_loop:
    lodsb
    or al, al
    jz disk_read_start
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp print_s1_loop

disk_read_start:
    ; 3. Disk Read: Load Stage 2 into memory
    mov ah, 0x02
    mov al, STAGE2_SECTORS
    mov ch, 0
    mov dh, 0
    mov cl, 2
    mov bx, STAGE2_OFFSET
    int 0x13
    jc disk_error

    ; 4. Transfer Execution to Stage 2
    jmp 0x0000:STAGE2_OFFSET

disk_error:
    mov si, disk_error_msg
error_print_loop:
    lodsb
    or al, al
    jz halt_loop
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0C
    int 0x10
    jmp error_print_loop

halt_loop:
    cli
    hlt
    jmp halt_loop

; Variables
BOOT_DRIVE db 0
stage1_msg db "Stage1 OK", 13, 10, 0
disk_error_msg db "Error: Disk read failed! Halting.", 0

    ; Padding and Magic Signature
    times 510 - ($ - $$) db 0
    dw 0xAA55
