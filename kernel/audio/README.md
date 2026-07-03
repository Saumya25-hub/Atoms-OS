# ATOMS OS Audio Core

This directory contains the Audio Subsystem Foundation for ATOMS OS. 
This is the core management layer that handles audio streams, software mixing, buffer management, and device registration.

## Architecture

- **audio_core**: Global stream registry and subsystem lifecycle.
- **audio_stream**: Individual audio session structures and state tracking.
- **audio_buffer**: Lock-free single-producer/single-consumer ring buffer for IPC and DMA.
- **audio_api**: High-level syscall bindings and state machine validation.
- **audio_debug**: Telemetry, leak tracking, and self-tests.

## Phase 7.2

Currently, this module implements the Foundation. There is NO hardware interaction, no DMA, and no decoding logic here. Those belong in future phases (Driver implementations, Mixer, Userspace media libs).
