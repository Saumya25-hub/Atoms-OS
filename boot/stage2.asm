; ==============================================================================
; SignaturesOS - Stage 2 Bootloader
; ==============================================================================
%include "boot/memory.inc"
%include "boot/boot.inc"

[ORG STAGE2_ADDR]
[BITS 16]

stage2_start:
    ; 1. Print "Stage2 OK"
    mov si, stage2_msg
    call print_str

    ; Checkpoint: STAGE2_A20_OK
    in al, 0x92
    or al, 2
    out 0x92, al

    mov si, chk_a20_msg
    call print_str

    ; Checkpoint: STAGE2_UNREAL_OK
    call ensure_unreal_mode

    mov si, chk_unreal_msg
    call print_str

load_kernel:
    ; Checkpoint: STAGE2_KERNEL_LOAD_BEGIN
    mov si, chk_load_begin_msg
    call print_str

    ; Initialize loader state variables in memory
    mov word [load_sectors_rem], KERNEL_SECTORS
    mov dword [load_lba_curr], KERNEL_LBA
    mov dword [load_dest_curr], KERNEL_EXEC
    mov word [load_chunk_idx], 0

load_kernel_loop:
    mov cx, [load_sectors_rem]
    cmp cx, 0
    je load_kernel_complete

    mov dx, cx
    cmp dx, 64
    jbe .set_dap
    mov dx, 64                          ; Read max 64 sectors per call (32 KB)

.set_dap:
    mov word [dap_kernel_sectors], dx
    mov word [dap_kernel_offset], 0x0000
    mov word [dap_kernel_segment], (KERNEL_BUFFER >> 4)   ; Buffer at 0x20000 (segment 0x2000)
    mov eax, [load_lba_curr]
    mov dword [dap_kernel_lba], eax
    mov dword [dap_kernel_lba + 4], 0

    ; Per-chunk checkpoint: print "C<idx>:R_"
    mov si, chk_chunk_prefix
    call print_str
    mov ax, [load_chunk_idx]
    call print_dec
    mov si, chk_read_msg
    call print_str

    ; Defensive BIOS INT 13h call with 3 retries
    mov bp, 3                           ; Retry counter

.try_int13:
    pusha
    xor ax, ax                          ; Ensure DS = 0, ES = 0 for BIOS call
    mov ds, ax
    mov es, ax
    mov ah, 0x42                        ; Extended Read
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]
    mov si, dap_kernel
    int 0x13
    jnc .read_success                   ; If Carry Flag clear, success!

    ; Read error: reset disk system (AH=00h) and retry
    popa
    xor ax, ax
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]
    int 0x13
    dec bp
    jnz .try_int13

    jmp kernel_read_error

.read_success:
    popa

    ; Per-chunk checkpoint: print "OK..CP_"
    mov si, chk_read_ok_msg
    call print_str

    ; RE-ESTABLISH UNREAL MODE BEFORE COPYING TO CURE ANY BIOS CLOBBERING OF FS CACHE
    call ensure_unreal_mode

    ; Load 32-bit pointers explicitly from memory variables
    mov esi, KERNEL_BUFFER              ; Source = 0x20000
    mov edi, [load_dest_curr]           ; Dest = 32-bit physical address (e.g. 0x00100000)
    movzx ecx, word [dap_kernel_sectors] ; Number of sectors read
    shl ecx, 7                          ; ecx = sectors * 512 / 4 (dwords to copy)

.copy_dword_loop:
    mov eax, [ds:esi]                   ; Read dword from low scratch buffer (0x20000)
    mov [fs:edi], eax                   ; Write dword to high memory (0x100000 + dest_offset)
    add esi, 4
    add edi, 4
    dec ecx
    jnz .copy_dword_loop

    ; Save updated 32-bit destination pointer back to memory variable
    mov [load_dest_curr], edi

    ; Update 32-bit LBA and remaining sectors in memory variables
    movzx edx, word [dap_kernel_sectors] ; dx = sectors transferred
    mov eax, [load_lba_curr]
    add eax, edx
    mov [load_lba_curr], eax

    mov ax, [load_sectors_rem]
    sub ax, dx
    mov [load_sectors_rem], ax

    inc word [load_chunk_idx]

    ; Per-chunk checkpoint: print "OK "
    mov si, chk_chunk_ok_msg
    call print_str

    jmp load_kernel_loop

load_kernel_complete:
    mov si, crlf_msg
    call print_str

    ; Checkpoint: STAGE2_KERNEL_LOAD_COMPLETE
    mov si, chk_load_complete_msg
    call print_str

    jmp memory_and_vbe

kernel_read_error:
    mov si, kernel_err_msg
    call print_str
halt_err:
    cli
    hlt
    jmp halt_err

ensure_unreal_mode:
    push eax
    cli                         ; Disable interrupts while switching CR0
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1                   ; Set PE (Protected Mode) bit
    mov cr0, eax

    ; Load 4GB segment selector into DS, ES, FS, and GS descriptor caches
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Clear PE bit to return to Real Mode
    and eax, ~1
    mov cr0, eax

    ; Reset DS and ES to segment 0x0000 with 0 base address for Real Mode operations
    ; In x86 Real Mode, loading DS/ES with 0 sets segment base to 0 while preserving the 4GB limit in hidden descriptor cache.
    xor ax, ax
    mov ds, ax
    mov es, ax

    sti                         ; Re-enable interrupts for BIOS calls
    pop eax
    ret

memory_and_vbe:
    sti                         ; Re-enable interrupts for BIOS E820 and VBE int calls

    ; Detect Physical Memory (E820)
    mov di, BOOT_INFO_ADDR + 32 ; First entry at BOOT_INFO_ADDR + 32
    xor ebx, ebx
    xor bp, bp                  ; Entry count
.e820_loop:
    mov eax, 0xE820
    mov ecx, 24
    mov edx, 0x534D4150         ; 'SMAP'
    mov dword [di + 20], 1      ; Default ACPI valid bit
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    jcxz .e820_skip
    add di, 24
    inc bp
.e820_skip:
    test ebx, ebx
    jz .e820_done
    jmp .e820_loop
.e820_done:
    ; Store entry count (32-bit) at BOOT_INFO_ADDR
    mov dword [BOOT_INFO_ADDR], ebp

    ; ----------------------------------------------------
    ; Detect and Setup VBE Graphics Mode (Priority List)
    ; ----------------------------------------------------
    mov di, 0x7000
    mov ax, 0x4F00
    mov dword [di], 0x32454256 ; 'VBE2'
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    mov si, vbe_search_msg
    call print_str

    mov si, vbe_table_header
    call print_str

    ; Initialize best tracking in scratch memory
    mov dword [0x7100], 0 ; best_score
    mov word [0x7104], 0  ; best_mode
    mov word [0x7106], 0  ; best_width
    mov word [0x7108], 0  ; best_height

    mov fs, word [0x7010] ; Segment of mode list
    mov si, word [0x700E] ; Offset of mode list

.vbe_mode_loop:
    mov cx, word [fs:si]  ; Read mode
    add si, 2
    cmp cx, 0xFFFF
    je .vbe_evaluate_best

    pusha
    mov ax, 0x4F01
    mov di, 0x7200
    int 0x10
    cmp ax, 0x004F
    jne .vbe_next_mode

    mov ax, [0x7200]
    and ax, 0x0090
    cmp ax, 0x0090
    jne .vbe_next_mode

    cmp byte [0x7219], 32 ; Must be 32 bpp
    jne .vbe_next_mode



    ; --- PRINT MODE DISCOVERY ---
    pusha
    mov si, vbe_mode_prefix
    call print_str
    mov ax, cx             ; CX holds the mode number
    call print_hex_word
    mov si, vbe_mode_sep
    call print_str
    mov ax, word [0x7212]  ; Width
    mov dx, word [0x7214]  ; Height
    mov bl, byte [0x7219]  ; BPP
    call print_vbe_mode
    popa
    ; --- END PRINT MODE DISCOVERY ---

    ; Compare with priority resolutions
    mov eax, 0               ; fallback base score
    
    mov bx, word [0x7212]    ; width
    mov dx, word [0x7214]    ; height

    ; Priority 1: 1920x1080
    cmp bx, 1920
    jne .chk_2
    cmp dx, 1080
    jne .chk_2
    mov eax, 1200
    jmp .calc_done

.chk_2:
    ; Priority 2: 1600x1200
    cmp bx, 1600
    jne .chk_3
    cmp dx, 1200
    jne .chk_3
    mov eax, 900
    jmp .calc_done

.chk_3:
    ; Priority 3: 1600x900
    cmp bx, 1600
    jne .chk_4
    cmp dx, 900
    jne .chk_4
    mov eax, 800
    jmp .calc_done

.chk_4:
    ; Priority 4: 1440x900
    cmp bx, 1440
    jne .chk_5
    cmp dx, 900
    jne .chk_5
    mov eax, 700
    jmp .calc_done

.chk_5:
    ; Priority 5: 1366x768
    cmp bx, 1366
    jne .chk_6
    cmp dx, 768
    jne .chk_6
    mov eax, 600
    jmp .calc_done

.chk_6:
    ; Priority 6: 1280x720
    cmp bx, 1280
    jne .chk_fallback
    cmp dx, 720
    jne .chk_fallback
    mov eax, 500
    jmp .calc_done

.chk_fallback:
    ; Fallback: score based on area so higher resolution is slightly preferred
    movzx eax, bx
    movzx ebx, dx
    mul ebx                  ; EDX:EAX = width * height
    mov ebx, 10000
    div ebx                  ; EAX = EDX:EAX / 10000
    ; Ensure fallback score doesn't exceed 499
    cmp eax, 500
    jl .calc_done
    mov eax, 499

.calc_done:
    ; Compare with best score
    cmp eax, dword [0x7100]
    jle .vbe_next_mode

    ; We found a new best mode!
    mov dword [0x7100], eax
    mov word [0x7104], cx
    mov bx, word [0x7212]
    mov word [0x7106], bx
    mov bx, word [0x7214]
    mov word [0x7108], bx

.vbe_next_mode:
    popa
    jmp .vbe_mode_loop

.vbe_evaluate_best:
    mov si, vbe_pause_msg
    call print_str
    
    ; Pause for keypress bypassed for automated boot debugging
    ; mov ah, 0x00
    ; int 0x16

    cmp word [0x7104], 0
    je vbe_error ; No valid mode found

    ; Print Selected Mode
    mov si, vbe_pref_msg
    call print_str

    mov si, vbe_found_msg
    call print_str
    mov ax, [0x7106]
    mov dx, [0x7108]
    mov bl, 32
    call print_vbe_mode

    ; Re-fetch the mode info so we can set it and copy it to BOOT_INFO_ADDR
    mov ax, 0x4F01
    mov cx, word [0x7104]
    mov di, 0x7200
    int 0x10

    mov cx, word [0x7104]
    or cx, 0x4000       ; Set LFB bit
    mov ax, 0x4F02
    mov bx, cx
    int 0x10
    cmp ax, 0x004F
    jne vbe_error

    ; Save VBE params to BOOT_INFO
    movzx eax, word [0x7212]
    mov dword [BOOT_INFO_ADDR + 4], eax
    movzx eax, word [0x7214]
    mov dword [BOOT_INFO_ADDR + 8], eax
    movzx eax, word [0x7210]
    mov dword [BOOT_INFO_ADDR + 12], eax
    movzx eax, byte [0x7219]
    mov dword [BOOT_INFO_ADDR + 16], eax
    mov dword [BOOT_INFO_ADDR + 20], 0 ; padding
    mov eax, dword [0x7228]
    mov dword [BOOT_INFO_ADDR + 24], eax
    mov dword [BOOT_INFO_ADDR + 28], 0

    mov si, vbe_fb_msg
    call print_str
    mov eax, dword [0x7228]
    call print_hex_dword

    mov si, vbe_pass_msg
    call print_str

    jmp vbe_done

print_hex_word:
    pusha
    mov bx, ax
    mov cx, 4
.hex_loop:
    rol bx, 4
    mov ax, bx
    and al, 0x0F
    cmp al, 9
    jle .hex_digit
    add al, 7
.hex_digit:
    add al, '0'
    mov ah, 0x0E
    push bx
    mov bh, 0x00
    int 0x10
    pop bx
    loop .hex_loop
    popa
    ret

print_hex_dword:
    pushad
    mov ebx, eax
    mov cx, 8
.hex_loop_dword:
    rol ebx, 4
    mov eax, ebx
    and al, 0x0F
    cmp al, 9
    jle .hex_digit_dword
    add al, 7
.hex_digit_dword:
    add al, '0'
    mov ah, 0x0E
    push ebx
    mov bh, 0x00
    int 0x10
    pop ebx
    loop .hex_loop_dword
    popad
    ret

print_dec:
    pusha
    mov cx, 0
    mov bx, 10
.loop1:
    mov dx, 0
    div bx
    push dx
    inc cx
    cmp ax, 0
    jne .loop1
.loop2:
    pop dx
    mov al, dl
    add al, '0'
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    loop .loop2
    popa
    ret

print_vbe_mode:
    ; AX=width, DX=height, BL=bpp
    pusha
    call print_dec
    mov al, 'x'
    mov ah, 0x0E
    int 0x10
    mov ax, dx
    call print_dec
    mov al, 'x'
    mov ah, 0x0E
    int 0x10
    xor ah, ah
    mov al, bl
    call print_dec
    mov si, crlf_msg
    call print_str
    popa
    ret

print_str:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

vbe_error:
    mov si, vbe_err_msg
    call print_str
.halt_verr:
    cli
    hlt
    jmp .halt_verr


vbe_done:
    mov si, chk_before_mode_switch_msg
    call print_str

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG:protected_mode_start

; Disk Address Packet (DAP) for Kernel Read
align 4
dap_kernel:
    db 0x10             
    db 0                
dap_kernel_sectors:
    dw 0               
dap_kernel_offset:
    dw 0x0000           
dap_kernel_segment:
    dw 0           
dap_kernel_lba:
    dq 0                

align 4
load_sectors_rem   dw 0
load_lba_curr      dd 0
load_dest_curr     dd 0
load_chunk_idx     dw 0

; ==============================================================================
; 32-Bit Global Descriptor Table (GDT)
; ==============================================================================
align 8                     
gdt_start:
    dq 0x0                  

gdt_code:                   
    dw 0xFFFF               
    dw 0x0                  
    db 0x0                  
    db 10011010b            
    db 11001111b            
    db 0x0                  

gdt_data:                   
    dw 0xFFFF               
    dw 0x0                  
    db 0x0                  
    db 10010010b            
    db 11001111b            
    db 0x0                  
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  
    dd gdt_start                

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ==============================================================================
; 32-Bit Protected Mode
; ==============================================================================
[BITS 32]
protected_mode_start:
    ; Reload segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Initialize 32-bit stack
    mov ebp, STACK_TOP
    mov esp, ebp

    ; Note: Kernel was already loaded directly to KERNEL_EXEC (0x100000) by Unreal Mode Chunked Loader

    ; Print "Protected Mode OK"
    mov ebx, pm_message
    mov edx, VGA_MEMORY

print_pm_loop:
    mov al, [ebx]
    cmp al, 0
    je setup_paging

    mov ah, 0x0E
    mov [edx], ax

    add ebx, 1
    add edx, 2
    jmp print_pm_loop

setup_paging:
    ; Zero out Page Tables (PML4, 1 PDP, 4 PDs = 24KB = 6144 dwords)
    mov edi, PAGE_TABLE_BASE        
    xor eax, eax            
    mov ecx, 6144
    rep stosd            

    ; Link Tables: PML4[0] -> PDP
    mov edi, PAGE_TABLE_BASE
    mov dword [edi], PAGE_TABLE_BASE + 0x1007

    ; Link PDP[0..3] -> PD0..PD3
    mov edi, PAGE_TABLE_BASE + 0x1000
    mov eax, PAGE_TABLE_BASE + 0x2007
    mov ecx, 4
link_pdp_loop:
    mov dword [edi], eax
    add eax, 0x1000
    add edi, 8
    loop link_pdp_loop

    ; Identity Map the full 4GB using 2MB Huge Pages in PD0..PD3
    mov edi, PAGE_TABLE_BASE + 0x2000        
    mov ebx, 0x00000087     ; Present | R/W | User | Huge (Bit 7)
    mov ecx, 2048           ; 2048 entries * 2MB = 4GB

build_pd_loop:
    mov dword [edi], ebx    
    add ebx, 0x200000       ; 2MB step
    add edi, 8              
    loop build_pd_loop

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5          
    mov cr4, eax

    ; Load CR3
    mov eax, PAGE_TABLE_BASE
    mov cr3, eax

    ; Print "Paging OK"
    mov ebx, paging_message
    mov edx, VGA_MEMORY + 160

print_paging_loop:
    mov al, [ebx]
    cmp al, 0
    je setup_long_mode

    mov ah, 0x0B
    mov [edx], ax

    add ebx, 1
    add edx, 2
    jmp print_paging_loop

setup_long_mode:
    ; Enable Long Mode (LME)
    mov ecx, 0xC0000080     
    rdmsr                   
    or eax, 1 << 8          
    wrmsr                   

    ; Enable Paging (PG)
    mov eax, cr0
    or eax, 1 << 31         
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64_descriptor]

    ; Far Jump to 64-bit Long Mode
    jmp CODE64_SEG:long_mode_start

; ==============================================================================
; 64-Bit Global Descriptor Table (GDT)
; ==============================================================================
align 8
gdt64_start:
    dq 0x0000000000000000   ; 0x00: Null
gdt64_code:
    dq 0x0020980000000000   ; 0x08: Kernel Code
gdt64_data:
    dq 0x0000920000000000   ; 0x10: Kernel Data
gdt64_user_data:
    dq 0x0000F20000000000   ; 0x18: User Data (Ring 3)
gdt64_user_code:
    dq 0x0020F80000000000   ; 0x20: User Code (Ring 3)
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

CODE64_SEG equ gdt64_code - gdt64_start
DATA64_SEG equ gdt64_data - gdt64_start
USER_CODE64_SEG equ gdt64_user_code - gdt64_start
USER_DATA64_SEG equ gdt64_user_data - gdt64_start

; ==============================================================================
; 64-Bit Long Mode
; ==============================================================================
[BITS 64]
long_mode_start:
    ; Reload segment registers
    mov ax, DATA64_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Print "Long Mode OK"
    mov rbx, lm_message
    mov rdx, VGA_MEMORY + 320

print_lm_loop:
    mov al, [rbx]
    cmp al, 0
    je jump_kernel

    mov ah, 0x0A
    mov [rdx], ax

    add rbx, 1
    add rdx, 2
    jmp print_lm_loop

jump_kernel:
    mov rbx, chk_before_jump_msg
    mov rdx, VGA_MEMORY + 480
.print_jump_loop:
    mov al, [rbx]
    cmp al, 0
    je .do_jump
    mov ah, 0x0E
    mov [rdx], ax
    inc rbx
    add rdx, 2
    jmp .print_jump_loop

.do_jump:
    mov rdi, BOOT_INFO_ADDR     ; Pass boot_info_t pointer to kernel via RDI
    mov rax, KERNEL_EXEC
    jmp rax

; Strings
stage2_msg db "Stage2 OK", 13, 10, 0
chk_a20_msg db "STAGE2_A20_OK", 13, 10, 0
chk_unreal_msg db "STAGE2_UNREAL_OK", 13, 10, 0
chk_load_begin_msg db "STAGE2_KERNEL_LOAD_BEGIN", 13, 10, 0
chk_chunk_prefix db "C", 0
chk_read_msg db ":R_", 0
chk_read_ok_msg db "OK..CP_", 0
chk_chunk_ok_msg db "OK ", 0
chk_load_complete_msg db 13, 10, "STAGE2_KERNEL_LOAD_COMPLETE", 13, 10, 0
chk_before_mode_switch_msg db "STAGE2_BEFORE_MODE_SWITCH", 13, 10, 0
chk_before_jump_msg db "STAGE2_BEFORE_KERNEL_JUMP", 0
kernel_err_msg db "Error: Kernel Read FAILED! Halting.", 13, 10, 0
pm_message db "Protected Mode OK", 0
paging_message db "Paging OK", 0
lm_message db "Long Mode OK", 0
vbe_search_msg db "Searching VBE Modes...", 13, 10, 0
vbe_pref_msg db 13, 10, "Preferred Resolution:", 13, 10, "1920x1080", 13, 10, 13, 10, 0
vbe_found_msg db "Selected Resolution:", 13, 10, 0
vbe_fb_msg db 13, 10, "Framebuffer:", 13, 10, "0x", 0
vbe_pass_msg db 13, 10, 13, 10, "PASS_VBE_SELECTION", 13, 10, 13, 10, 0
crlf_msg db 13, 10, 0
vbe_err_msg db "Error: No suitable VBE Mode FOUND! Halting.", 0
vbe_table_header db "VBE Discovery", 13, 10, 0
vbe_mode_prefix db "Mode 0x", 0
vbe_mode_sep db ": ", 0
vbe_pause_msg db "Press any key to boot...", 13, 10, 0
    times (STAGE2_SECTORS * BOOT_SECTOR_SIZE) - ($ - $$) db 0
