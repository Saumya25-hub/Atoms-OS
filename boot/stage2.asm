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
print_loop:
    lodsb
    or al, al
    jz load_kernel
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0A
    int 0x10
    jmp print_loop

load_kernel:
    ; Load Kernel via Extended LBA (AH=42h) in chunks
    ; VirtualBox/BIOS limits INT 13h AH=42h to 127 sectors max. We use 64.
    mov cx, KERNEL_SECTORS              ; Remaining sectors
    mov ax, (KERNEL_BUFFER >> 4)        ; Current segment
    mov ebx, KERNEL_LBA                 ; Current LBA

load_kernel_loop:
    cmp cx, 0
    je disk_read_success

    mov dx, cx
    cmp dx, 64
    jbe .do_read
    mov dx, 64                          ; Read max 64 sectors per call

.do_read:
    mov word [dap_kernel_sectors], dx
    mov word [dap_kernel_segment], ax
    mov dword [dap_kernel_lba], ebx

    pusha
    mov ah, 0x42
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]
    mov si, dap_kernel
    int 0x13
    jc kernel_error
    popa

    sub cx, dx                          ; cx -= dx
    add ebx, edx                        ; ebx += dx (LBA)
    
    shl dx, 5                           ; dx * 32 (512 bytes / 16 bytes per segment)
    add ax, dx                          ; Next segment

    jmp load_kernel_loop

disk_read_success:

    ; Print "Disk Read OK"
    mov si, disk_ok_msg
print_disk_ok:
    lodsb
    or al, al
    jz enable_a20
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0A
    int 0x10
    jmp print_disk_ok

kernel_error:
    mov si, kernel_err_msg
print_kerr:
    lodsb
    or al, al
    jz halt_err
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0C
    int 0x10
    jmp print_kerr
halt_err:
    cli
    hlt
    jmp halt_err

enable_a20:
    cli

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

    ; DEVELOPMENT BUILD ONLY
    ; Force 1920x1080 until Display Manager is implemented.
    cmp word [0x7212], 1920
    jne .vbe_next_mode
    cmp word [0x7214], 1080
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

    ; Calculate Aspect Ratio = (width * 10) / height
    movzx eax, word [0x7212]
    mov ebx, 10
    mul ebx
    movzx ebx, word [0x7214]
    cmp ebx, 0
    je .vbe_next_mode
    xor edx, edx
    div ebx
    
    ; eax now holds aspect ratio indicator (17, 16, 13, 12)
    mov edx, 0               ; default bonus
    cmp eax, 17
    jne .check_16
    mov edx, 5000            ; 16:9 huge bonus
    jmp .calc_base_score
.check_16:
    cmp eax, 16
    jne .check_13
    mov edx, 3000            ; 16:10 large bonus
    jmp .calc_base_score
.check_13:
    cmp eax, 13
    jne .check_12
    mov edx, 500             ; 4:3 minor bonus
    jmp .calc_base_score
.check_12:
    cmp eax, 12
    jne .calc_base_score
    mov edx, 0               ; 5:4 no bonus

.calc_base_score:
    ; Base score = (width * height) / 1000
    movzx eax, word [0x7212]
    movzx ebx, word [0x7214]
    mul ebx                  ; eax = width * height
    mov ebx, 1000
    push edx
    xor edx, edx
    div ebx                  ; eax = base score
    pop edx
    
    add eax, edx             ; eax = total score

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
    ; 3. Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; CRITICAL: Disable interrupts before entering Protected Mode!
    ; BIOS calls (int 10h, int 16h) re-enable interrupts. If an IRQ fires
    ; while in Protected Mode before the kernel sets up the IDT, the CPU Triple Faults.
    cli

    ; 4. Load GDT & Enter Protected Mode
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
    dw 0x0000           
dap_kernel_segment:
    dw 0           
dap_kernel_lba:
    dq 0                

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

    ; Copy Kernel from KERNEL_BUFFER to KERNEL_EXEC
    mov esi, KERNEL_BUFFER        
    mov edi, KERNEL_EXEC       
    mov ecx, (KERNEL_SECTORS * BOOT_SECTOR_SIZE) / 4           
    rep movsd               

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
    ; Zero out Page Tables (PML4, PDP, PD)
    mov edi, PAGE_TABLE_BASE        
    xor eax, eax            
    mov ecx, 3072
    rep stosd            

    ; Link Tables
    mov edi, PAGE_TABLE_BASE
    mov dword [edi], PAGE_TABLE_BASE + 0x1007 ; PML4[0] -> PDP

    mov edi, PAGE_TABLE_BASE + 0x1000
    mov dword [edi], PAGE_TABLE_BASE + 0x2007 ; PDP[0] -> PD

    ; Identity Map the first 1GB using 2MB Huge Pages in PD
    mov edi, PAGE_TABLE_BASE + 0x2000        
    mov ebx, 0x00000087     ; Present | R/W | User | Huge (Bit 7)
    mov ecx, 512            ; 512 entries * 2MB = 1GB

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
    mov rdi, BOOT_INFO_ADDR     ; Pass boot_info_t pointer to kernel via RDI
    mov rax, KERNEL_EXEC
    jmp rax

; Strings
stage2_msg db "Stage2 OK", 13, 10, 0
disk_ok_msg db "Disk Read OK", 13, 10, 0
kernel_err_msg db "Error: Kernel Read FAILED! Halting.", 0
pm_message db "Protected Mode OK", 0
paging_message db "Paging OK", 0
lm_message db "Long Mode OK", 0
vbe_search_msg db "Searching VBE Modes...", 13, 10, 0
vbe_found_msg db "Selected: ", 0
crlf_msg db 13, 10, 0
vbe_err_msg db "Error: No suitable VBE Mode FOUND! Halting.", 0
vbe_table_header db "--- VBE Mode Discovery Engine ---", 13, 10, 0
vbe_mode_prefix db "Mode 0x", 0
vbe_mode_sep db ": ", 0
vbe_pause_msg db "Press any key to boot...", 13, 10, 0
    times (STAGE2_SECTORS * BOOT_SECTOR_SIZE) - ($ - $$) db 0
