
with open('boot/stage2.asm', 'r') as f:
    code = f.read()

# 1. Add A20 and Unreal Mode before load_kernel
unreal_setup = '''
    ; Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Enable Unreal Mode
    cli
    push ds
    push es
    lgdt [gdt_descriptor]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp $+2
    mov bx, DATA_SEG
    mov ds, bx
    mov es, bx
    and al, 0xFE
    mov cr0, eax
    jmp $+2
    pop es
    pop ds
    sti

load_kernel:
    mov dword [kernel_dest_addr], KERNEL_EXEC
'''
code = code.replace('load_kernel:', unreal_setup)

# 2. Modify load_kernel_loop
new_loop = '''load_kernel_loop:
    cmp cx, 0
    je disk_read_success

    mov dx, cx
    cmp dx, 64
    jbe .do_read
    mov dx, 64                          ; Read max 64 sectors per call

.do_read:
    mov word [dap_kernel_sectors], dx
    mov word [dap_kernel_segment], (KERNEL_BUFFER >> 4)
    mov dword [dap_kernel_lba], ebx

    pusha
    mov ah, 0x42
    mov dl, [BOOT_ADDR + BOOT_SECTOR_SIZE - 4]
    mov si, dap_kernel
    int 0x13
    jc kernel_error
    popa

    ; Copy chunk to high memory (Unreal Mode)
    pusha
    push ds
    push es
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov esi, KERNEL_BUFFER
    mov edi, dword [kernel_dest_addr]
    movzx ecx, dx
    shl ecx, 7   ; dx * 128 (number of dwords)
    rep movsd
    
    movzx edx, dx
    shl edx, 9   ; dx * 512
    add dword [kernel_dest_addr], edx
    
    pop es
    pop ds
    popa

    sub cx, dx                          ; cx -= dx
    movzx edx, dx
    add ebx, edx                        ; ebx += dx (LBA)
    
    jmp load_kernel_loop
'''

import re
code = re.sub(r'load_kernel_loop:.*?jmp load_kernel_loop', new_loop, code, flags=re.DOTALL)

# 3. Add kernel_dest_addr variable
dap_replacement = '''dap_kernel_lba:
    dq 0                
kernel_dest_addr:
    dd KERNEL_EXEC
'''
code = code.replace('dap_kernel_lba:\n    dq 0', dap_replacement)

# 4. Remove Protected Mode copy
pm_copy = '''    ; Copy Kernel from KERNEL_BUFFER to KERNEL_EXEC
    mov esi, KERNEL_BUFFER        
    mov edi, KERNEL_EXEC       
    mov ecx, (KERNEL_SECTORS * BOOT_SECTOR_SIZE) / 4           
    rep movsd               '''
code = code.replace(pm_copy, '    ; Kernel is already at KERNEL_EXEC via Unreal Mode')

# 5. Remove old A20 enable if it's there
old_a20 = '''    ; 3. Enable A20 Line
    in al, 0x92
    or al, 2
    out 0x92, al'''
code = code.replace(old_a20, '    ; A20 already enabled earlier')


with open('boot/stage2.asm', 'w') as f:
    f.write(code)
print('Patched stage2.asm!')

