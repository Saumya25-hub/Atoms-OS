# ATOMS OS Foundation v2 Audio Subsystem
## 04: Driver Interface

To achieve modularity and support future hardware (HDA, USB, Bluetooth), all audio drivers must implement a strict, standardized contract. 

### The `audio_driver_ops` Interface

Every driver will define a structure containing function pointers for the Audio Device Manager to call.

```c
struct audio_caps {
    uint32_t max_sample_rate;
    uint8_t channels;           // 1 = Mono, 2 = Stereo
    uint8_t bit_depth;          // 8, 16, 24, 32
    bool supports_hardware_mix; // True if hardware handles multiple streams
};

struct audio_driver_ops {
    // Initialization and Teardown
    int (*initialize)(void);
    int (*shutdown)(void);

    // Playback Control
    int (*play)(void);
    int (*stop)(void);
    int (*pause)(void);
    int (*resume)(void);

    // Configuration
    int (*set_volume)(uint8_t volume); // 0-255 scaling
    int (*query_caps)(struct audio_caps* caps);

    // Data Transfer
    int (*stream)(void* buffer, size_t size);
    int (*flush)(void);

    // Interrupt Management
    void (*interrupt_handler)(void);
};
```

### API Responsibilities

*   `initialize()`: Probes the hardware, maps memory/IO ports, configures DMA, and resets the device to a known good state. Returns 0 on success.
*   `shutdown()`: Halts playback, frees DMA memory, and unmaps IO ports. Used during OS shutdown or driver unloads.
*   `play()`: Starts the DMA engine or hardware timer for audio output.
*   `stop()`: Halts the hardware and resets the buffer pointers.
*   `pause()`: Temporarily suspends hardware playback without resetting pointers.
*   `resume()`: Resumes hardware playback from the paused state.
*   `set_volume()`: Scales the global hardware output volume. Software streams have independent volume handled by the Mixer.
*   `query_caps()`: Populates the provided struct with the hardware's capabilities so the Mixer knows how to format its output.
*   `stream()`: Queues PCM data into the driver's internal buffer or DMA region.
*   `flush()`: Drops all pending data in the hardware queue.
*   `interrupt_handler()`: The function called by the kernel's IRQ dispatcher. It must clear the hardware interrupt bit and signal the upper layers that more data is needed.
