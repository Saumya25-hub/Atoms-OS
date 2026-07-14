# PHASE 14 — DUPLICATE WINDOW CREATION AUTOPSY

## Objective
Determine why two independent 640x400 DOOM windows exist in the Window Manager (Window 4107 and Window 4108).

## Runtime Proof & Telemetry
We instrumented `BOS_CreateWindow` and `BOS_CreateSurface` in the kernel to trace every window creation and log the Return Address, Parent ID, Dimensions, and Title.

### The Timeline Log
```text
qemu_doom_test11.log:599:--- PHASE 14 AUTOPSY: Window Created ---
qemu_doom_test11.log:600:Window ID: 4107
qemu_doom_test11.log:601:Parent ID: 0
qemu_doom_test11.log:602:Width: 640
qemu_doom_test11.log:603:Height: 400
qemu_doom_test11.log:604:Return Addr: 0x0x109255
qemu_doom_test11.log:607:Window Title Set: DOOM

...

qemu_doom_test11.log:625:--- PHASE 14 AUTOPSY: Window Created ---
qemu_doom_test11.log:626:Window ID: 4108
qemu_doom_test11.log:627:Parent ID: 0
qemu_doom_test11.log:628:Width: 640
qemu_doom_test11.log:629:Height: 400
qemu_doom_test11.log:630:Return Addr: 0x0x109255
qemu_doom_test11.log:633:Window Title Set: DOOM
```

We also found this in the kernel logs:
```text
qemu_doom_test11.log:596:[PASS] doom_main entered (DG_Init)
...
qemu_doom_test11.log:622:[PASS] doom_main entered (DG_Init)
```

## Forensic Analysis

**1. The Return Address:**
Both windows have the exact same return address inside the kernel (`0x109255`), and both are named "DOOM". This return address belongs to the system call handler (`kernel/core/syscall/src/syscall.c:225`):
```c
bwe_error_t err = BOS_CreateWindow((int32_t)arg1, (int32_t)arg2, (int32_t)arg3, (int32_t)arg4, (const char*)arg5, &win_id);
```
This proves that the duplicate windows are not created by a bug inside the kernel's Window Manager, but rather by **two independent userspace processes making the same syscall**.

**2. Two DOOM Processes:**
The `[PASS] doom_main entered (DG_Init)` telemetry prints twice, confirming that `DOOM.ELF` is launched twice!

**3. The Culprit:**
We traced the kernel initialization sequence in `kernel/kernel.c` and found exactly where the double spawn occurs.

**First Spawn (via Desktop Shell):**
`kernel.c:677` calls `Desktop_Shell_Initialize()`.
Inside `desktop_shell.c:783`, it explicitly launches DOOM via the Horse Engine:
```c
horse_launch(APP_ID_DOOM); // Spawns DOOM.ELF
```

**Second Spawn (Hardcoded in Kernel):**
Immediately after `Desktop_Shell_Initialize()` returns, `kernel.c:693` manually loads and spawns a second instance of DOOM!
```c
// kernel/kernel.c : 690
ProcessImage* new_image = elf_load_image(new_pml4, "/DOOM.ELF");
if (new_image) {
    if (process_build_user_stack(new_image, new_pml4)) {
        process_spawn(new_image, "DOOM.ELF"); // SPAWNS DOOM.ELF AGAIN!
        display_print("[SUCCESS] Spawned DOOM.ELF\n");
    }
// ...
```

## Conclusion
The duplicate window is created because **the kernel boots two identical, independent DOOM processes**. 

- Process 1 (Spawned by Desktop Shell) calls `BOS_CreateWindow` -> Creates Window 4107
- Process 2 (Spawned manually by kernel.c) calls `BOS_CreateWindow` -> Creates Window 4108

Because they are both DOOM, they both render to their own windows. SurfacePresent may update one window's canvas, while the compositor rendering loop is rendering the other window!
