# Audio Core Foundation - Ring Buffer

## Design
The `AudioRingBuffer` provides a safe, FIFO mechanism for transferring PCM data from the userspace application to the kernel mixer.

### Lock-Free Operation
The buffer is designed for Single-Producer / Single-Consumer scenarios.
*   **Producer**: The userspace application pushes data, updating the `head` pointer.
*   **Consumer**: The kernel mixer pulls data, updating the `tail` pointer.
Because the producer and consumer only ever update their respective pointers, the buffer is inherently lock-free as long as memory ordering is respected.

### API Capabilities
*   `audio_buffer_write`: Copies incoming bytes into the ring buffer, handling wrap-around at the end of the allocated block. Returns the actual number of bytes written (clamped to free space).
*   `audio_buffer_read`: Reads bytes out of the ring buffer, handling wrap-around. Returns the actual number of bytes read.
*   `audio_buffer_available`: Calculates how many unread bytes are currently in the buffer.
*   `audio_buffer_free_space`: Calculates how much space is left. *Note: The buffer reserves 1 byte of space to mathematically distinguish between completely full and completely empty states (head == tail).*

### Allocation
By default, the Stream Manager allocates a 16KB buffer for each new stream, sufficient to prevent underruns while maintaining low latency.
