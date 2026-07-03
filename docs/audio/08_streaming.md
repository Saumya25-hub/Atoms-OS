# ATOMS OS Foundation v2 Audio Subsystem
## 08: Streaming

Audio streaming is the process of continuously feeding data into the audio subsystem for prolonged playback, such as background music or video soundtracks.

### The Streaming Lifecycle

#### 1. Initialization
An application calls `horse_audio_stream_create()`. The kernel allocates an `AudioStream` and a ring buffer. The app receives a handle to this stream.

#### 2. Pre-filling (Buffering)
Before playback starts, the app pushes chunks of PCM data into the stream using `horse_audio_stream_write()`. This pre-fills the ring buffer, preventing an immediate underrun when playback begins.

#### 3. Playback State
The app calls `horse_audio_stream_start()`. The mixer includes this stream in its accumulation loop.

#### 4. The Feeding Loop
While playing, the app must continuously provide data. It can do this in two ways:
*   **Polling (Non-blocking)**: The app calls a status API to see how much space is available in the ring buffer, then writes that exact amount.
*   **Blocking**: The app writes a large chunk of data. The `write()` syscall blocks the calling thread until there is enough space in the ring buffer.

### Handling Underruns
If the app thread is starved for CPU time or blocked on disk I/O, the ring buffer will empty out. 
*   **Mixer Behavior**: The mixer will detect the empty buffer and substitute silence (zeros) for that stream.
*   **App Notification**: The telemetry engine logs an underrun event. The app can query its stream state and choose to pause, re-buffer, and resume to avoid audio stuttering.

### Background Music Support
Because ATOMS OS is a multitasking system, streaming audio can continue even if the app loses focus. The app simply needs a worker thread that occasionally wakes up to refill the buffer. The kernel handles the continuous output seamlessly.
