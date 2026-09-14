# ATOMS OS — Master System Architecture & Interconnection Map
**Status:** Audit & Architecture Map Only (No Code Changes)  
**Date:** July 16, 2026

---

## 1. High-Level Architecture Diagram (Subsystem Interconnections)

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                           ATOMS OS KERNEL                                              │
│                                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────────────────────┐  │
│  │                              APPLICATION & SHELL LAYER (Ring 0 / Ring 3)                         │  │
│  │                                                                                                  │  │
│  │   ┌──────────────────────────┐       ┌──────────────────────────┐       ┌────────────────────┐   │  │
│  │   │      Desktop Shell       │       │        DOOM Engine       │       │    Audio Test /    │   │  │
│  │   │  (Taskbar, Icons, Apps)  │       │     (3D Game / Process)  │       │  Diagnostic Mode   │   │  │
│  │   └────────────┬─────────────┘       └────────────┬─────────────┘       └─────────┬──────────┘   │  │
│  └────────────────┼──────────────────────────────────┼─────────────────────────────────┼──────────────┘  │
│                   │                                  │                                 │               │
│                   │ (Events / Surfaces)              │ (Framebuffers / Input / I/O)    │ (VFS / API)   │
│                   ▼                                  ▼                                 ▼               │
│  ┌─────────────────────────────────┐   ┌──────────────────────────┐   ┌─────────────────────────┐  │
│  │      Window Manager (BWE)       │   │       VFS & FAT32        │   │     Audio Session       │  │
│  │                                 │   │                          │   │   (audio_player.c)      │  │
│  │  • Compositor (Dirty rects)     │   │  • FD Table (32 slots)   │   │                         │  │
│  │  • Surface Management           │   │  • Cluster Chain Walker  │   │  • Producer Task        │  │
│  │  • Hit-Testing & Dispatcher     │   │  • Per-Handle Cache      │   │    (AudioSvc @ 20ms)    │  │
│  └────────────────▲────────────────┘   └─────────────┬────────────┘   └────────────┬────────────┘  │
│                   │                                  │                             │               │
│                   │ (GUI Events)                     │ (Block I/O)                 │ (PCM Chunks)  │
│                   ▼                                  ▼                             ▼               │
│  ┌─────────────────────────────────┐   ┌──────────────────────────┐   ┌─────────────────────────┐  │
│  │           Input Core            │   │      Storage Driver      │   │   256 KB Ring Buffer    │  │
│  │                                 │   │      (Legacy ATA PIO)    │   │  (SPSC Circular Buffer) │  │
│  │  • Pointer Engine (Scale/Acc)   │   │                          │   └────────────┬────────────┘  │
│  │  • Raw PS/2 & Mouse Packets     │   │  • `rep insw` hardware   │                │               │
│  │  • Event Queues                 │   │  • `cli` / `sti` blocks  │                │ (Mixer Read)  │
│  └────────────────▲────────────────┘   └─────────────▲────────────┘                ▼               │
│                   │                                  │                ┌─────────────────────────┐  │
│                   │ (Hardware IRQ 12)                │ (Hardware I/O) │       Audio Mixer       │  │
│                   │                                  │                │  • Volume & Normalizer  │  │
│                   │                                  │                └────────────┬────────────┘  │
│                   │                                  │                             │               │
│                   │                                  │                             │ (PCM Fill)    │
│                   │                                  │                             ▼               │
│  ┌────────────────┼──────────────────────────────────┼─────────────────┐┌─────────────────────────┐  │
│  │                │          SCHEDULER & IRQ CORE        │                 ││   AC97 DMA Manager    │  │
│  │  ┌─────────────┴─────────────┐    ┌───────────────┴──────────────┐  ││  (128 KB / 32 Descs)  │  │
│  │  │  IRQ 12 (Mouse Interrupt) │    │  IRQ 0 (PIT Timer @ 1000Hz)  │  │└────────────┬────────────┘  │
│  │  └───────────────────────────┘    └───────────────┬──────────────┘  │             │               │
│  │                                                   │                 │             │ (Bus Master)  │
│  │                                                   ▼                 │             ▼               │
│  │                                     ┌──────────────────────────────┐│┌─────────────────────────┐  │
│  │                                     │  audio_realtime_worker_pump  │││    Hardware Registers   │  │
│  │                                     │  (Runs directly inside IRQ0) │││    (CIV, LVI, SR, CR)   │  │
│  │                                     └──────────────────────────────┘│└─────────────────────────┘  │
│  └─────────────────────────────────────────────────────────────────────┘                               │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Who is Connected to Whom? (Subsystem Interaction Matrix)

| From Subsystem | To Subsystem | Connection Medium / Mechanism | What Data Moves across? |
| :--- | :--- | :--- | :--- |
| **Hardware Mouse** | **Input Core** | `IRQ 12` Hardware Interrupt (`ps2_mouse.c`) | Raw 3-byte / 4-byte movement and button packets |
| **Input Core** | **Window Manager (BWE)** | `bos_gui_event_push` / Pointer Engine | Absolute/Relative coordinates (`x, y`), Click states (`pressed/released`) |
| **Window Manager** | **Desktop Shell / Apps** | Event Dispatcher (`bwe_surface_hit_test`) | Mouse click delivery to specific window IDs (`shell_window_id`, `doom_window_id`) |
| **AudioSvc Task** | **VFS / FAT32 Driver** | `vfs_read()` (`fd` table lookup) | File data chunks (`DEMO1.WAV` or `BOOT1.WAV`) |
| **VFS / FAT32** | **ATA PIO Driver** | `block_device_read()` (`ata_insw`) | Sector reads (512 bytes per sector via `rep insw` with interrupts disabled) |
| **AudioSvc Task** | **256 KB Ring Buffer** | `audio_stream_write()` | 16 KB decoded PCM audio packets pushed to `head` pointer |
| **IRQ 0 Timer Tick** | **Realtime Audio Pump** | Direct function call (`timer_tick_handler` → `audio_realtime_worker_pump`) | High-priority 1000 Hz wakeup pulse |
| **Audio Pump (IRQ 0)** | **Audio Mixer** | `audio_mixer_process()` | Reads from 256 KB ring buffer (`tail`), mixes, and outputs to DMA buffer |
| **Audio Mixer** | **AC97 DMA Buffer** | Memory copy into `bdl[idx].buffer_phys_addr` | 4,096 bytes of 16-bit Stereo PCM data per descriptor |
| **AC97 DMA Engine** | **Physical AC97 Codec** | PCI Bus Master DMA (`BDBAR`, `CIV`, `LVI` registers) | Direct physical RAM DMA transfer to hardware audio DAC |

---

## 3. What Are We Missing? (The "Blind Spots" / Root Causes)

When we look at how the system is wired together, **three major architectural blind spots (missing links)** explain why audio breaks after 2–3 seconds and why mouse clicks fail or drop:

---

### 🚨 Blind Spot #1: Virtual vs. Physical Contiguity in DMA (`kmalloc` vs Hardware Bus Master)
* **What happens right now:**  
  When `ac97_dma_prepare(131072)` allocates the 128 KB DMA buffer for the AC97 audio card, it uses standard `kmalloc(131072)`. Then, it calculates physical addresses for the 32 BDL descriptors assuming that if virtual pages are consecutive, physical frames in RAM must also be consecutive:
  $$\text{Assumed Phys} = \text{pcm\_buffer\_phys} + (i \times 4096)$$
* **Why it breaks (`The 2–3 Second Static Bug`):**  
  When ATOMS OS first boots (`BOOT1.WAV` or start of `DEMO1.WAV`), the heap allocator (`heap.c`) has fresh memory. A large `kmalloc` gets **physically contiguous memory by pure luck**.  
  However, after **2–3 seconds**, DOOM loads (`elf_load_image`), desktop windows open, and the heap gets fragmented across different physical page frames. Once physical memory is no longer contiguous, `kmalloc(131072)` returns consecutive *virtual addresses*, but **random, scattered physical memory pages**!
* **The Result:**  
  The AC97 hardware Bus Master reads purely from **physical addresses**. When the 32 descriptors point to incorrect physical memory (e.g., kernel page tables, filesystem buffers, or uninitialized RAM), the hardware plays whatever bytes are sitting there — producing loud **continuous static/noise (`"shhhhh"`)**.
* **The Missing Piece:**  
  We lack a **Physical Page Frame Allocator (`pmm_alloc_contiguous_pages`)** guaranteed to return physically contiguous RAM blocks for DMA hardware.

---

### 🚨 Blind Spot #2: Interrupt Starvation & Concurrency Collision (`IRQ 0 vs ATA PIO Disk Reads`)
* **What happens right now:**  
  The Audio Consumer is driven directly by **IRQ 0 (PIT Timer @ 1000 Hz)** (`timer_tick_handler` → `audio_realtime_worker_pump`). It must run strictly every **1 millisecond** to check hardware `CIV` rotation and refill the mixer.  
  However, when the Audio Producer (`AudioSvc` task) or DOOM calls `vfs_read()` to read a file from disk, the ATA driver (`ata.c`) executes sector reads using:
  ```c
  __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags)); // DISABLES ALL INTERRUPTS!
  ata_insw(io_base + ATA_REG_DATA, ptr, 256);             // Reads sectors from disk
  if (flags & 0x200) __asm__ volatile("sti");             // Re-enables interrupts
  ```
* **Why it breaks:**  
  When reading 16 KB of audio or loading DOOM assets, `cli` (Clear Interrupts) blocks **all CPU interrupts** (`IRQ 0 Timer` and `IRQ 12 Mouse`) during the transfer.  
  If the disk read takes 15–30 milliseconds (`cli` held or repeatedly grabbed under spinlock without preemption), **IRQ 0 is completely frozen during that time**.
* **The Result:**  
  1. **Audio Starvation:** Because `IRQ 0` stops firing, `ac97_playback_update()` misses hardware descriptor rotations (`CIV`). The hardware exhausts all 32 descriptors (`LVI`), halts (`DCH` bit set), or plays stale data. By the time interrupts re-enable, the mixer output buffer is out-of-sync or starved.  
  2. **Mouse Freezes/Drops:** Mouse packets (`IRQ 12`) arriving during disk reads get delayed or dropped from the PS/2 controller buffer.
* **The Missing Piece:**  
  We need **DMA-based or Asynchronous Interrupt-driven ATA storage (`UDMA / IDE IRQ 14/15`)** instead of blocking `PIO + cli` polling inside kernel tasks.

---

### 🚨 Blind Spot #3: Shared Static Scratch Buffers Without Locking (`fat32.c`)
* **What happens right now:**  
  Inside `fat32.c`, hot cluster reading uses a global static scratch buffer:
  ```c
  static uint8_t s_cluster_scratch[32768]; // 32 KB global buffer
  ```
* **Why it breaks:**  
  If DOOM or the Desktop Shell is reading assets from `VFS` while the background `AudioSvc` task wakes up and reads audio chunks from `DEMO1.WAV`, there is **no lock (`mutex` or `spinlock`) on `s_cluster_scratch[]` across files**.
* **The Result:**  
  Two concurrent reads overwrite `s_cluster_scratch[]` mid-transfer. The audio producer ends up copying DOOM textures or directory structures into the 256 KB audio ring buffer — causing bursts of screeching noise or corrupting data!
* **The Missing Piece:**  
  Per-handle or dynamically allocated/locked read buffers inside VFS/FAT32 to prevent multi-task clobbering.

---

### 🚨 Blind Spot #4: Coordinate Translation & Z-Ordering Disconnect (Why Mouse Clicks Miss Start / Apps)
* **What happens right now:**  
  The Mouse driver (`ps2_mouse.c`) passes relative movement to `InputCore`, which maintains a logical cursor coordinate (`x, y`).  
  However, `Window Manager (BWE)` hit-testing (`bwe_surface_hit_test`) and `compositor` rendering do not share an enforced, unified coordinate bounding box or strict Z-order hierarchy when full-screen applications (`DOOM` surface vs `Taskbar` BWE surface) overlap.
* **Why it breaks:**  
  1. When DOOM or an app window opens, or when scaling/resolution mapping (`absolute vs relative` sensitivity) drifts between pointer space and surface space, the cursor visible to the user (`x_vis, y_vis`) does not equal the internal hit-test box (`x_hit, y_hit`).  
  2. Clicking on the Start Button or taskbar apps sends the click coordinate to whatever BWE surface claims the Z-index at `(x, y)`. If an invisible background surface, unclipped BWE border, or mis-scaled bounding box consumes the click event first, the target application/start menu never receives the event (`bos_gui_event_pop`).

---

## 4. Summary Table: Symptoms & Architectural Cause

| Visible Symptom | Architectural Missing Link | Root Cause |
| :--- | :--- | :--- |
| **Audio plays clean for 2–3 sec, then turns to continuous static ("shhhhh")** | **Blind Spot #1:** Physical Memory Contiguity (`kmalloc` vs `PMM`) | Initial memory allocation is physically contiguous. After 2–3s, heap allocation fragments. AC97 DMA Bus Master reads non-contiguous physical pages containing garbage. |
| **Audio stutters/stalls when DOOM or large files read from disk** | **Blind Spot #2:** Blocking PIO ATA reads (`cli` disabling IRQ 0) | `cli` disables IRQ 0 during `rep insw` sector reads. The 1000 Hz realtime audio pump freezes, missing hardware CIV descriptor rotations. |
| **Sudden loud screeching / corrupted sound during multi-tasking** | **Blind Spot #3:** Unsynchronized global `s_cluster_scratch[32768]` | `AudioSvc` task and application file reads clobber each other's data inside the static FAT32 buffer. |
| **Mouse cursor moves & clicks in kernel, but Start Button / Apps don't click** | **Blind Spot #4:** Disconnected coordinate scaling & BWE hit-testing | Mismatch between Pointer Engine logical coordinates (`x,y`) and BWE surface boundaries (`bwe_surface_hit_test`), or overlapping Z-order consuming click events. |
