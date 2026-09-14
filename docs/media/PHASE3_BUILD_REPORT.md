# ATOMS OS — Phase 3 Build Report

**Date**: 2026-09-12  
**Build Host**: Windows (Cross-compilation to x86_64-unknown-none / x86_64-pc-none-elf)  

---

## Kernel Objects (Ring-0)

| Object | Compiler | Flags | Status |
|--------|----------|-------|--------|
| `build/vfs.o` | clang (x86_64-unknown-none) | `-ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -msoft-float` | ✅ |
| `build/services.o` | clang (x86_64-unknown-none) | same | ✅ |
| `build/audio_core.o` | clang (x86_64-unknown-none) | same | ✅ |
| `build/process_manager.o` | clang (x86_64-unknown-none) | same | ✅ |

## Userspace Objects (Ring-3)

| Object | Compiler | Flags | Status |
|--------|----------|-------|--------|
| `build/bos_media_stream.o` | clang (x86_64-pc-none-elf) | `-ffreestanding -nostdlib -Ithird_party/musl/include -Ithird_party/musl/arch/x86_64 -Ithird_party/musl/arch/generic` | ✅ |
| `build/bos_media_avio.o` | clang (x86_64-pc-none-elf) | same | ✅ |
| `build/bos_audio_stream.o` | clang (x86_64-pc-none-elf) | same | ✅ |
| `build/bos_media_pipeline.o` | clang++ -std=c++20 (x86_64-unknown-none-elf) | `-ffreestanding -fno-pie -fno-pic -mcmodel=small -mno-red-zone -fno-exceptions -fno-rtti -O2` | ✅ |

## Library Archive

| Archive | Contents | Status |
|---------|----------|--------|
| `build/libbos_media.a` | bos_media_stream.o, bos_media_avio.o, bos_audio_stream.o, bos_media_pipeline.o + Phase 1/2 objects | ✅ |

## Final Linked Binaries

| Binary | Size (bytes) | Linker | Status |
|--------|-------------|--------|--------|
| `build/media_player.elf` | 354,528 | ld.lld with `userspace/linker.ld` | ✅ |
| `build/embedded_desktop_elf.o` | ~354K (incbin) | nasm elf64 | ✅ |
| `build/kernel.bin` | 19,953,984 | ld.lld with `kernel/linker.ld` via `build/link.rsp` | ✅ |
| `build/BOOTX64.EFI` | 19,964,928 | clang PE/COFF (efi_application) | ✅ |

## ELF Memory Map (media_player.elf)

| Section | Address Range | Notes |
|---------|--------------|-------|
| `.text` | 0x40000000 → ~0x4003A4C9 | Code segment |
| `.bss` | — → ~0x400CB234 | 64KB audio FIFO + 128KB sample buffer |
| Stack guard | 0x400FB000 | — |
| **Headroom** | ~191 KB | ✅ Safe — no stack collision risk |

## GPT Disk Image

| Field | Value |
|-------|-------|
| Image | `build/atoms_uefi_test.img` |
| Size | 536,870,912 bytes (512 MB) |
| Partitioning | GPT + EFI System Partition (FAT32) |
| Contents | BOOTX64.EFI, KERNEL.BIN, STARTUP.NSH, TEST.MP4, HEROES.MP3, MEDIA.IMG, MEDIA.ELF |
| TEST.MP4 | 49,065,277 bytes |
| HEROES.MP3 | 3,329,709 bytes |
| MEDIA.ELF | 354,528 bytes |

## Build Errors

**0 errors. 1 warning** (benign empty body warning in bootx64.c serial wait loop).
