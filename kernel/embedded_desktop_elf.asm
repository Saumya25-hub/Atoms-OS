[bits 64]
global g_embedded_desktop_elf
global g_embedded_desktop_elf_len

section .data
align 16
g_embedded_desktop_elf:
    incbin "build/desktop_shell.elf"
g_embedded_desktop_elf_end:

align 8
g_embedded_desktop_elf_len:
    dq g_embedded_desktop_elf_end - g_embedded_desktop_elf
