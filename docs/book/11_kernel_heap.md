# Chapter 11: Kernel Dynamic Heap Allocator

The kernel heap provides dynamic memory allocation (`kmalloc`, `kfree`, `kcalloc`) for driver structures, VFS nodes, network packets, and compositor surfaces.

## 1. Allocator Structure
- **Boundary Tag Coalescing**: Each allocated and free chunk contains a header and footer storing size and allocation flags, allowing $O(1)$ bidirectional coalescing on `kfree`.
- **Alignment**: Every allocated buffer is 16-byte aligned for SSE/AVX compatibility.
- **Expansion**: When the current heap region is exhausted, the allocator automatically requests additional physical pages from the PMM and maps them into the kernel virtual heap space via the VMM.

## 2. Debug Hardening
In debug builds, the heap includes guard canaries and memory poisoning:
- Unallocated/freed memory is poisoned with `0xDD` to detect use-after-free bugs.
- Newly allocated memory is poisoned with `0xAA` to reveal uninitialized variable reads.
- Chunk boundary underflow/overflow canaries (`0xDEADBEEFCAFEBABE`) are validated on every allocation and deallocation.
