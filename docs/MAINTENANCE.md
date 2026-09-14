# ATOMS OS — Repository Maintenance & Engineering Standards

This document establishes the repository structure, code organization standards, and maintenance rules for ATOMS OS. All future modifications, automated scripts, and contributions must strictly comply with these rules.

---

## 1. The Repository Directory Standard

The repository enforces strict separation of concerns across eight primary categories:

```
Atoms-OS/
├── [ROOT]         -> Pristine project entry point only
├── boot/          -> UEFI and BIOS bootloaders (Firmware interface)
├── kernel/        -> BOS Kernel Ring 0 core, memory, interrupts, drivers, VFS
├── arch/          -> Architecture-specific low-level assembly and hardware setup (x86_64)
├── drivers/       -> Hardware driver stacks (USB, Storage, Network, Audio, Display)
├── userspace/     -> Ring 3 runtime libraries, Rook login supervisor, Desktop Shell
├── apps/          -> Native user-facing applications (DOOM, Media Player, Dashboards)
├── third_party/   -> External vendor libraries, headers, and reference drivers
├── tools/         -> Developer tooling, build automation, QEMU runners, image builders
├── tests/         -> Automated test suites, unit tests, integration harnesses, test data
├── docs/          -> Canonical book, architectural specs, certifications, history
├── assets/        -> Static media (wallpapers, icons, fonts, branding, boot audio)
└── build/         -> Ephemeral generated binaries, objects, logs, and disk images
```

---

## 2. Strict Placement Rules

### Rule 1: Root Directory Pristine Standard
The repository root must **never** contain temporary files, disassembly dumps, serial logs, one-off test scripts, or random markdown notes.
- **Allowed in Root**:
  - `README.md` (Project overview and welcome)
  - `LICENSE` / `LICENSE.md` (Legal public license)
  - `CHANGELOG.md` (Formal version changelog)
  - `RELEASE_NOTES.md` (Milestone release notes)
  - `.gitignore` (Git ignore rules)
  - `.gn` / `BUILD.gn` (GN build configuration)
  - `build.ps1` (Primary full build pipeline)
  - Canonical subsystem directories (`kernel/`, `boot/`, `docs/`, etc.)
- **Prohibited in Root**:
  - ❌ `.log` files (Must be directed to `build/logs/`)
  - ❌ `.txt` dumps and symbol tables (Must go to `tools/dumps/`)
  - ❌ `.py` patch scripts (Must go to `tools/` or `tests/`)
  - ❌ `.vmdk`, `.vdi`, `.img`, `.bin`, `.o` (Must go to `build/`)
  - ❌ Milestone/investigation reports (Must go to `docs/milestones/` or `docs/history/`)

### Rule 2: Scripts and Tooling
All utility scripts must be classified by purpose under `tools/`:
- `tools/build/`: Build generation, compilation helpers.
- `tools/qemu/`: Hypervisor launching, OVMF UEFI execution scripts.
- `tools/boot/`: Bootloader packaging, Bochs configuration.
- `tools/image/`: Partition table generators, disk formatters (`gpt_image_builder.c`).
- `tools/diagnostics/`: Serial log analyzers, frame inspectors, audio frequency validators.
- `tools/development/`: Asset converters, font generators, icon builders.
- `tools/maintenance/`: Repository health checkers, path fixers.
- `tools/maintenance/archive/`: Historical, one-off migration and patch scripts.

### Rule 3: Automated Tests & Verification
All test code must be located in `tests/`:
- `tests/unit/`: Standalone host-compiled unit tests (data structures, codecs, parsers).
- `tests/integration/`: Multi-module test harnesses (filesystem WAL, VFS mounts).
- `tests/kernel/`: Low-level kernel property tests (structure offsets, alignment checks).
- `tests/boot/`: Automated boot regression scripts.
- `tests/data/`: Test media samples, virtual disk images, audio streams.
- `tests/bin/`: Compiled host test executables.

### Rule 4: Generated Artifacts & Ephemeral Files
All build outputs belong strictly in `build/` (which is tracked by `.gitignore`):
- `build/*.o`: Compiled intermediate object files.
- `build/kernel.bin`: Linked 64-bit BOS Kernel binary.
- `build/BOOTX64.EFI`: Compiled UEFI PE32+ application.
- `build/*.elf`: Compiled userspace ELF executables.
- `build/*.img`: Bootable raw/GPT disk images.
- `build/*.vdi` / `build/*.vmdk`: Virtual machine disk containers.
- `build/logs/`: Serial output logs and compiler traces.

---

## 3. Third-Party Code Isolation

External libraries must remain strictly isolated inside `third_party/` to avoid license contamination:
- **Never rename or re-license third-party files**.
- Every third-party library must maintain its upstream `LICENSE` or `COPYING` file.
- Document third-party provenance in `docs/third_party/`:
  - `docs/third_party/ATOMS_CHROMIUM_PROVENANCE.md` (Chromium / Blink)
  - `docs/third_party/ATOMS_SKIA_PROVENANCE.md` (Skia 2D Graphics Engine)
  - `docs/third_party/ATOMS_V8_PROVENANCE.md` (Google V8 JavaScript Engine)
  - `docs/third_party/ATOMS_THIRDPARTY_LICENSES.md` (musl, FFmpeg, libmpv)

---

## 4. Release Preparation & Git Cleanliness

Before tagging a new milestone or committing changes:
1. **Clean Workspace**: Ensure no stray files exist in the repository root (`git status`).
2. **Build Verification**: Run `.\build.ps1` cleanly with 0 errors.
3. **QEMU Pre-Flight**: Execute `.\tools\qemu\run_qemu_test.ps1` and verify full boot sequence through to desktop blitting.
4. **Documentation Sync**: If architectural changes were made, update `docs/START_HERE.md` and the relevant chapter in `docs/book/`.
5. **Atomic Commits**: Group related changes logically. Do not squash historical commits or rewrite history.
