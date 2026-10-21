# FORENSIC_REPORT.md — FreeBSD Init Hang at start_init
## Date: 2026-10-21
## Author: Atoms OS Forensic Team

---

## 1. Symptom & Evidence

### Symptom
On both bare-metal H81 physical hardware and QEMU, the Atoms OS Type-1 Hypervisor successfully loads the FreeBSD 14.1-RELEASE guest kernel, mounts the root filesystem from VirtIO-BLK (`/dev/vtbd0`), prints:
```text
Dual Console: Serial Primary, Video Secondary
start_init: trying /sbin/init
```
and then hangs with a blinking cursor. The live shell banner and interactive root shell `# ` prompt never appear.

### Forensic Telemetry Evidence (AMDE UDP Port 9999)
1. **VM is Active & Healthy**:
   The telemetry heartbeat log confirms the vCPU is running in non-root mode with active timer interrupts:
   ```text
   [HEARTBEAT 5s] Pending=0 Injected=0 RTE_BLK=0xA031 ISR=0x0 Used=105 Avail=105 TotalHLT=0 RDMSR=68508 WRMSR=98323 Exits=[48,32,32,32,1]
   ```
   - `Used=105, Avail=105`: All 105 VirtIO-BLK disk requests completed successfully.
   - `WRMSR` continues to increment at ~20 Hz due to FreeBSD writing `MSR 0x6E0` (IA32_TSC_DEADLINE).
   - `TotalHLT=0`: The guest is NOT halted in a crash panic; it is running an active loop.

2. **Zero Bytes Emitted After `start_init`**:
   The last message captured on COM1 UART before the hang was `start_init: trying /sbin/init\n`. No kernel panic (`panic: /sbin/init exited`), no signal 11, and no dynamic linker errors occurred.

---

## 2. Root Cause Analysis

Disassembly of `/sbin/init` (authentic FreeBSD 14.1-RELEASE static ELF64 binary, entry `0x23d1e0`) revealed three interlocking fatal bottlenecks:

### Root Cause 1: `ttyu0` DCD (Carrier Detect) Block in `/etc/ttys`
In `tools/freebsd_payload/minimal_rootfs/etc/ttys`, line 43:
```text
ttyu0   "/bin/sh"   vt100   on secure
```
`ttyu0` is marked `on`. When `init` parses `/etc/ttys` during `clean_ttys()` (vaddr `0x23fa80`), it invokes `start_getty()` (`0x23eb20`) on `ttyu0`. In FreeBSD, opening `/dev/ttyu0` requires the physical DCD (Data Carrier Detect, Pin 1) signal to be asserted high. On the physical H81 motherboard, there is no null-modem cable with asserted DCD connected to COM1. Consequently, opening `ttyu0` blocks indefinitely in the kernel's `uart(4)` driver, halting userland progression.

### Root Cause 2: Overlapping `console` and `ttyv0` Conflict in `/etc/ttys`
In `/etc/ttys`, lines 29 and 31:
```text
console "/bin/sh"   vt100   on secure
ttyv0   "/bin/sh"   xterm   on secure
```
In FreeBSD, `console` is a meta-alias and MUST be `off` when virtual terminal `ttyv0` is `on`. Having both `on` causes `init` to attempt to spawn two competing `/bin/sh` processes on the same underlying display device.

### Root Cause 3: `/etc/rc` Missing Execution Loop & Permission
1. In `tools/freebsd_payload/minimal_rootfs/etc/rc`, the script echoes the banner and terminates at EOF (line 18). It does not spawn or `exec /bin/sh`.
2. In `tools/make_minimal_ufs2.py`, line 170:
   ```python
   (7, "rc", os.path.join(SOURCE_ROOTFS, "etc", "rc"), False),
   ```
   `is_exec` is set to `False` (`0644`), causing `access("/etc/rc", X_OK)` to fail with `EACCES`.
3. If `/etc/rc` reaches EOF, `init` transitions to `clean_ttys` where it hits the `ttyu0` DCD lockup.

### Root Cause 4: Primary Console Selection in `freebsd_loader.c`
In `kernel/core/hypervisor/src/freebsd_loader.c`, line 160:
```c
"console=vidconsole,comconsole",
```
FreeBSD negotiates `Serial Primary, Video Secondary` when serial probe responds. For physical display and keyboard operation on the H81 motherboard, `vidconsole` must be prioritized. Setting `init_script=/etc/rc` in kernel environment ensures `/sbin/init` explicitly targets `/etc/rc`.

---

## 3. Files Involved
1. `d:\Signatures_OS\tools\freebsd_payload\minimal_rootfs\etc\ttys` — Terminal definitions
2. `d:\Signatures_OS\tools\freebsd_payload\minimal_rootfs\etc\rc` — Startup script & interactive shell
3. `d:\Signatures_OS\tools\make_minimal_ufs2.py` — File permissions in UFS2 rootfs
4. `d:\Signatures_OS\kernel\core\hypervisor\src\freebsd_loader.c` — Loader environment variables

---

## 4. Risk Analysis
- **Low Risk**: Adjusting `/etc/ttys` to disable `ttyu0` and `console`, leaving `ttyv0` active. This is standard FreeBSD configuration for non-serial systems.
- **Low Risk**: Adding `exec /bin/sh` to `/etc/rc` ensures the process never exits and stays as an active live shell.
- **Zero Kernel Architecture Risk**: No hypervisor core CPU/EPT logic is touched.

---

## 5. Suspected Fix (NO CODE)
1. In `etc/ttys`: Disable `ttyu0` (`off`) and `console` (`off`), keep `ttyv0` (`on`).
2. In `etc/rc`: Add `exec /bin/sh` so `/etc/rc` transitions directly into the interactive root shell.
3. In `make_minimal_ufs2.py`: Mark `rc` as executable (`is_exec=True`).
4. In `freebsd_loader.c`: Set `init_script=/etc/rc` in `env_vars`.
