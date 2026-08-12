section .rodata
global g_kernel_data
global g_kernel_size_val

g_kernel_data:
    incbin "build/kernel.bin"
g_kernel_data_end:

g_kernel_size_val:
    dq g_kernel_data_end - g_kernel_data
