# Decoder Pipeline

The decoder pipeline defines a common signature:
	ypedef struct BOSSurface* (*BopawnDecoderFunc)(const uint8_t* buffer, uint32_t size);

Each format (PNG, BMP, RAW, ICO) implements this interface. The pipeline routes the buffer to the correct decoder based on the format detection logic.