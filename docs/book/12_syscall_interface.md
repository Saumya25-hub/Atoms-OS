# Chapter 12: Hardware Fast Syscalls (IA32_LSTAR)

Privilege transitions between Ring 3 userspace and Ring 0 kernel space occur via the x86_64 fast syscall hardware gateway (`SYSCALL` and `SYSRET`).

## 1. MSR Configuration
During boot, the kernel programs CPU Model-Specific Registers:
- **`IA32_STAR (0xC0000081)`**: Encodes target GDT selectors for kernel CS/SS and user CS/SS.
- **`IA32_LSTAR (0xC0000082)`**: Stores the 64-bit virtual entry point address of `syscall_entry` in [`kernel/core/syscall/`](file:///D:/Signatures_OS/kernel/core/syscall).
- **`IA32_FMASK (0xC0000084)`**: Masks RFLAGS on entry (clearing `IF` to disable interrupts, clearing `TF` and `DF`).
- **`IA32_KERNEL_GS_BASE (0xC0000102)`**: Holds pointer to the CPU core data structure, allowing `SWAPGS` to swap user GS and kernel GS atomically.

## 2. Standard Syscall Registry
| Syscall Number | Name | Function Signature | Description |
| :---: | :--- | :--- | :--- |
| `0` | `SYS_EXIT` | `void sys_exit(int code)` | Terminate active process. |
| `1` | `SYS_WRITE` | `int sys_write(int fd, const char *buf, size_t len)` | Write buffer to file/stdout. |
| `2` | `SYS_READ` | `int sys_read(int fd, char *buf, size_t len)` | Read bytes from file/stdin. |
| `9` | `SYS_MMAP` | `void *sys_mmap(void *addr, size_t len, int prot, int flags)` | Allocate virtual memory pages. |
| `10` | `SYS_MPROTECT` | `int sys_mprotect(void *addr, size_t len, int prot)` | Change page protection flags. |
| `18` | `SYS_SHOW_WINDOW` | `int sys_show_window(uint32_t win_id, int state)` | Request BCM damage/present. |
| `20` | `SYS_MAP_SURFACE` | `void *sys_map_surface(uint32_t win_id, ...)` | Map compositor client surface. |
| `24` | `SYS_SET_WALLPAPER`| `int sys_set_wallpaper(const void *buf, ...)` | Update desktop wallpaper buffer. |
