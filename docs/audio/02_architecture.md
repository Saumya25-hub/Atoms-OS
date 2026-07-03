# ATOMS OS Foundation v2 Audio Subsystem
## 02: Architecture

### Folder Hierarchy Blueprint
The audio subsystem will be integrated into the OS tree as follows:

```text
kernel/
  audio/
    audio_core.c        # Main entry point and lifecycle management
    audio_dev_mgr.c     # Device registration, enumeration, hot-plugging
    audio_mixer.c       # Software multi-channel mixing
    audio_buffer.c      # Ring buffers and memory management
    audio_stream.c      # Stream instances and continuous playback engine
  drivers/
    audio/
      audio_driver.h    # Standardized hardware contract
      hda/              # High Definition Audio (Future)
      ac97/             # AC97 Audio (Future)
      sb16/             # SoundBlaster 16 (Future)
  engine/
    audio_api.c         # Horse Engine audio bindings for applications
apps/
  lib/
    media/
      pcm_decoder.c     # Raw PCM handling
      wav_decoder.c     # WAV file parsing
      mp3_decoder.c     # MP3 stream decoding
```

### Module Responsibilities

#### Audio Core (`audio_core.c`)
Acts as the central nervous system. It initializes the subsystem, coordinates between the mixer and devices, and provides the syscall handlers for userspace communication.

#### Audio Device Manager (`audio_dev_mgr.c`)
Maintains a registry of all active audio hardware. When the OS enumerates PCI/USB devices, matching drivers call into this manager to register themselves. It handles default device selection and hot-plugging logic.

#### Audio Mixer (`audio_mixer.c`)
Because most hardware only exposes a single playback channel via DMA, this module performs software mixing. It takes multiple incoming PCM streams, adjusts their volume, resamples them to the hardware's native rate if necessary, and sums the samples before sending them to the driver.

#### Audio Ring Buffer (`audio_buffer.c`)
Provides lock-free (or lightweight spinlock) circular buffer implementations. These are used to safely transfer PCM data from the userspace Application Thread into the kernel's Audio Worker Thread without blocking.

#### Audio Stream Engine (`audio_stream.c`)
Manages the lifecycle of an individual audio request. A "stream" is an active session between an application and the mixer. It tracks playback state (Playing, Paused, Stopped), volume, and buffer pointers.

#### Userspace Decoders (`apps/lib/media/`)
The kernel only understands raw PCM data. Applications are responsible for linking against media libraries that decode WAV, MP3, or OGG files into PCM *before* sending them to the Horse Engine.
