# Audio Core Foundation - Architecture

## Overview
The Audio Core (Phase 7.2) is the foundational subsystem of the ATOMS OS audio stack. It is explicitly designed *without* hardware drivers, decoders, or mixing logic. Its sole responsibility is **Subsystem Management**.

## Module Breakdown

1. **Audio Core (`audio_core.c`)**: Maintains the global registry of all active audio sessions (streams) across the entire OS.
2. **Audio Stream (`audio_stream.c`)**: Manages the lifecycle and state of a single audio session, tracking its state machine and allocating its resources.
3. **Audio Buffer (`audio_buffer.c`)**: A lock-free ring buffer used to transfer PCM data between the application and the kernel mixer (mixer to be implemented in Phase 7.3).
4. **Audio API (`audio_api.c`)**: The high-level kernel syscall boundary. It validates all state transitions and protects the core from malformed requests.
5. **Audio Debug (`audio_debug.c`)**: A dedicated telemetry engine that monitors memory allocation, peak stream counts, and provides a built-in software self-test.

## Design Philosophy
*   **Single Responsibility**: Each module does one thing. The core manages the list; the stream manages the struct; the buffer manages memory.
*   **Zero-Leak Enforcement**: Resources are strictly hierarchically owned. If a parent is destroyed, the child must be destroyed.
*   **Defensive API**: The API layer silently handles invalid requests (e.g. pausing a destroyed stream) to prevent user-space apps from crashing the kernel.
