# AUDIT PART 7: GUEST KERNEL & USERLAND COMPATIBILITY

## 1. Exact Binary Versions Identified

### 1.1 Embedded ATOMS Guest Kernel
- [VERIFIED-BY-CODE: kernel/embedded_freebsd_elf.asm:9]
- [VERIFIED-BY-CODE: tools/freebsd_payload/freebsd_stripped_kernel.elf]
- **Binary Inspection**:
  - Exact Release String: `@(#)FreeBSD 14.1-RELEASE releng/14.1-n267679-10e31f0946d8 GENERIC`
  - Kernel Architecture: `FreeBSD amd64 (ELF64)`
  - Kernel Version Code: `1401000` (`__FreeBSD_version`)
  - Build Date: `May 2024`

### 1.2 Userland in `FreeBSD_Runtime.img`
- [VERIFIED-ON-HARDWARE: User physical test photo & init log message]
- **Observed Behavior**:
  - Root filesystem contains an authentic FreeBSD 14.5 / 14-STABLE installation.
  - Expected Version Code: `1405000` (`__FreeBSD_version`)

---

## 2. Root Cause of `getting pipebuf resource limit: invalid argument`

- **The Error String**:
  `INIT 17 - - getting pipebuf resource limit: invalid argument`
- **Architectural Explanation**:
  1. In FreeBSD, process resource limits are managed by `getrlimit(int which, struct rlimit *rlp)` and `setrlimit()`.
  2. In **FreeBSD 14.1-RELEASE** (`sys/sys/resource.h`), the kernel defines resource limits 0 through 14:
     - `RLIMIT_CPU` (0) through `RLIMIT_UMTXP` (14).
     - The internal kernel limit count is `RLIM_NLIMITS = 15`.
  3. In **FreeBSD 14.5 / 15-CURRENT**, a new resource limit was introduced to bound memory consumed by pipes and FIFOs:
     ```c
     #define RLIMIT_PIPEBUF 15 /* pipes/fifos buffers */
     ```
  4. When the FreeBSD 14.5 `init` binary (`/sbin/init`) launches on PID 1, it queries its initial pipe buffer budget:
     `getrlimit(RLIMIT_PIPEBUF, &limit);` (passing `which = 15`).
  5. The embedded FreeBSD 14.1 kernel receives syscall `getrlimit(15, ...)`.
  6. The 14.1 kernel validates `which`:
     `if (which < 0 || which >= RLIM_NLIMITS) return (EINVAL);`
     Because in 14.1 `RLIM_NLIMITS == 15`, passing `15` fails the check and returns `EINVAL` (Error 22: `Invalid argument`).
  7. FreeBSD 14.5 `init` detects the error return and logs:
     `getting pipebuf resource limit: invalid argument`.

---

## 3. Kernel & Subsystem Features Needed by FreeBSD 14.5 Userland, Xorg & Firefox

| Subsystem / Feature | Required by Userland / App | Present in Embedded 14.1 Kernel? | Impact of Kernel Mismatch |
|:---|:---|:---|:---|
| **`RLIMIT_PIPEBUF` (Limit 15)** | `init`, `sh`, Firefox IPC pipes | **NO** (Returns `EINVAL`) | Non-fatal warning in init; may restrict pipe sizing in Firefox IPC. |
| **`kmod` Module Version ABI** | Xorg video drivers, sound drivers | **MISMATCH** (`1401000` vs `1405000`) | **FATAL FOR MODULES**: Any `.ko` kernel module built on 14.5 will fail to load with `module_register: version mismatch`. |
| **`evdev` Event Interface** | Xorg `xf86-input-libinput` | YES (Compiled in GENERIC) | Requires virtual input devices to register as `/dev/input/eventX`. |
| **Direct Rendering (`drm-kmod`)**| Hardware 2D/3D acceleration | **NO** (Not compiled into stripped kernel) | Xorg must fall back to unaccelerated `scfb` (Syscons Framebuffer) or software rasterization. |
| **`tmpfs`** | `/tmp`, `/var/run` runtime sockets | YES (Compiled in GENERIC) | Used by `/etc/rc` stage D0 for in-memory `/var`. |
| **Sound (`snd_hda` / OSS)** | Firefox HTML5 audio, YouTube | **NO** (Not compiled into stripped kernel) | Audio will be silent unless virtual sound device (AC97/HDA) and kernel modules are added. |
| **Extended Syscalls (`copy_file_range`, futex)**| Modern Firefox multi-process DOM | PARTIAL | Potential `ENOSYS` or fallback to userspace emulation loops. |

---

## 4. Can the Image's Own Kernel Be Loaded Instead?

### Current Limitation
- ATOMS OS embeds `tools/freebsd_payload/freebsd_stripped_kernel.elf` (14.1) directly into the `BOOTX64.EFI` bootloader executable at link time (`kernel/embedded_freebsd_elf.asm`).
- It does NOT load the guest kernel from `/boot/kernel/kernel` inside the virtual disk image.

### Feasibility of Loading Image's Native Kernel
1. **Option A: Pre-Extraction at Build Time (Cleanest & Recommended)**:
   - Extract `/boot/kernel/kernel` from `FreeBSD_Runtime.img` (14.5).
   - Strip debug symbols: `objcopy --strip-debug kernel freebsd_14_5_kernel.elf`.
   - Update `kernel/embedded_freebsd_elf.asm` to incbin the 14.5 ELF kernel.
   - **Result**: Zero userland mismatch, `RLIMIT_PIPEBUF` supported natively, module ABI matches.
2. **Option B: Runtime UFS2 Kernel Extraction (Complex)**:
   - ATOMS would need a full UFS2 filesystem driver inside the host bootloader to mount `vtbd0p2`, locate `/boot/kernel/kernel`, allocate GPA `0x400000`, and parse ELF headers.
   - High architectural risk; unnecessary when Option A is available.

---

## 5. Top 10 Claims Least Sure About (Part 7)

1. Whether Firefox 159 on FreeBSD 14.5 strictly requires `machdep.fpu_save` AVX2 instructions or can run on SSE2.
2. Whether Xorg on the image is configured to use `driver "scfb"` or `driver "modesetting"` by default.
3. If `/etc/fstab` on `vtbd0p2` specifies `rw` mount options, whether FreeBSD remounts root read-only cleanly when writes fail.
4. Exactly which dynamic linker (`ld-elf.so.1`) version is installed on `FreeBSD_Runtime.img`.
5. Whether `sysctl kern.ipc.pipebuf` exists in 14.1 as a global tunable instead of a per-process rlimit.
6. Whether Firefox crashes on startup if `/dev/dsp` (OSS audio) returns `ENOENT`.
7. How FreeBSD's `devfs` ruleset on `FreeBSD_Runtime.img` permissions `/dev/vtbd0` when running as an unprivileged user.
8. Whether the 14.1 kernel has the required sysctls for Chromium/Gecko sandboxing (`security.bsd.unprivileged_idp`).
9. If FreeBSD 14.5 `libc` uses any new AVX-512 optimized `memcpy` routines that fault on Haswell/RaptorLake.
10. Whether the userland on the USB image is a pure official FreeBSD release or a custom user-compiled build.
