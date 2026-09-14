# SignaturesOS Phase 15 — ConHost & Preemptive Scheduler Integration

## Executive Summary
During Phase 15, native `.BOSX` executable execution encountered severe interactivity failure: launching `SHELL.BOSX` produced a blank terminal window containing only a cursor caret (`_`), ignored keyboard input, and froze user interactivity. 

Through forensic kernel debugging, two profound architectural flaws were discovered and permanently resolved without altering core design principles:
1. **Process/Console Race Condition:** Userspace tasks were executing before their GUI terminal sessions were initialized.
2. **Uninitialized Preemptive Multitasking:** The primary GUI system task (`kernel_main`) never registered itself with the scheduler, leaving preemptive context switching disabled (`scheduler_running = false`).

---

## Technical Architectural Changes

### 1. Preemptive Scheduler Bootstrap (`kernel/scheduler/`)
* **Problem:** `kernel_main` entered its infinite GUI event loop directly without invoking `scheduler_start()`. Because `current_task` remained `NULL` and `scheduler_running` remained `false`, the timer interrupt (`timer_tick_handler`) returned immediately on every tick. User processes placed in `ready_queue` were starved indefinitely.
* **Solution:** Implemented `scheduler_register_boot_task()`. Called right before the GUI event loop in `kernel_main`, this function:
  * Allocates a dynamic kernel task structure representing the running GUI System Task.
  * Allocates a dedicated Ring 0 kernel stack (`KERNEL_TASK_STACK_SIZE`) ensuring hardware TSS (`TSS.RSP0`) has a valid stack pointer during Ring 3 privilege transitions.
  * Sets `current_task = boot_task` and activates preemptive switching (`scheduler_running = true`).

### 2. Atomic Console Session Spawning (`kernel/loader/bosx_loader.c`)
* **Problem:** When `BOSX_Load()` spawned a new process via `process_spawn()`, the task was immediately pushed to `ready_queue`. Because timer interrupts were active, the hardware timer preempted the loader and executed `SHELL.BOSX` **before** `conhost_spawn_console_for_process()` could create the window and attach the console FIFO. When `SHELL.BOSX` executed `bos_print()`, lookup failed, output dumped to serial stdio, and the process deadlocked waiting on uninitialized keyboard buffers.
* **Solution:** Wrapped process spawning and ConHost session attachment inside atomic CPU interrupt boundaries (`__asm__ volatile("cli")` and `"sti"`). This guarantees that user tasks cannot execute until their terminal windows and I/O buffers are fully instantiated.

### 3. Native Console Host Subsystem (`kernel/conhost/`)
* Established a decoupled, multi-session console server capable of managing up to 16 concurrent virtual terminal sessions (`CONHOST_MAX_SESSIONS`).
* Integrated bidirectional ring buffers (`stdin_buffer`) bridging raw window keyboard events to ring 3 `SYS_GET_KEY_EVENT` system calls.

---

## Verification & Results
* **Boot Validation:** Successfully built 64MB FAT32 image (`OS.img` / `SignaturesOS.vdi`).
* **Execution:** Clicking `SHELL.BOSX` in Explorer immediately opens a terminal window displaying full startup telemetry (Atoms Library self-tests, BishopMath checks, and interactive command prompt).
* **Interactivity:** Keyboard input flows seamlessly from GUI events -> ConHost FIFO -> Ring 3 Shell Process.
