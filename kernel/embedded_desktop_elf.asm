[bits 64]
global g_embedded_desktop_elf
global g_embedded_desktop_elf_len
global get_embedded_desktop_elf_len

section .rodata
align 16
g_embedded_desktop_elf:
    incbin "build/desktop_shell.elf"
g_embedded_desktop_elf_end:

align 16
g_embedded_desktop_elf_len:
    dq g_embedded_desktop_elf_end - g_embedded_desktop_elf

section .text
align 16
get_embedded_desktop_elf_len:
    mov rax, [rel g_embedded_desktop_elf_len]
    test rax, rax
    jnz .done
    mov rax, g_embedded_desktop_elf_end - g_embedded_desktop_elf
.done:
    ret
