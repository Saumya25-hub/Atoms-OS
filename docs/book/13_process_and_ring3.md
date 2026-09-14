# Chapter 13: Process Lifecycle & Ring 3 Runtime

Ring 3 userspace provides isolated process execution protecting the kernel from application failures.

## 1. Process Control Block (PCB)
Each task is represented by a PCB storing:
- `pid`: Unique Process Identifier.
- `cr3`: Physical base address of the process PML4 page table.
- `kernel_rsp`: Kernel stack pointer loaded upon syscall/interrupt entry.
- `user_rsp`: Userspace stack pointer restored upon `SYSRET`.
- `user_rip`: Instruction pointer in userspace.
- `state`: Process state (`READY`, `RUNNING`, `BLOCKED`, `ZOMBIE`).
- `fd_table`: Array of open VFS file descriptors.

## 2. ELF64 Loading Protocol
1. Open executable binary from VFS.
2. Validate ELF magic: `0x7F 'E' 'L' 'F'`, 64-bit class, little-endian.
3. Create new address space via `vmm_create_address_space()`.
4. Iterate Program Headers (`PT_LOAD`): allocate physical frames, map virtual addresses with requested flags (`PF_R`, `PF_W`, `PF_X`), copy segment data.
5. Allocate userspace stack (typically 2 MB at `0x00007FFFF0000000`).
6. Set CPU registers and execute `SYSRET` or `IRETQ` to jump to `e_entry` in Ring 3.
