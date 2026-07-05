# Architecture

The BOPAWN architecture follows a modular pipeline:
1. **VFS Integration**: Reads file buffers.
2. **Format Detection**: Checks magic headers (e.g., 0x89504E47 for PNG).
3. **Decoding**: Specific decoders process the raw bytes.
4. **Conversion**: The decoded RGBA data is converted into a BOSSurface.
5. **Caching**: LRU and reference counting prevent redundant decoding.