# PNG Decoder

The PNG decoder is fully native, meaning no libpng or zlib. It features:
- Chunk parsing (IHDR, IDAT, PLTE, IEND).
- A native RFC 1951 DEFLATE implementation (inflate.c).
- Scanline unfiltering (Sub, Up, Average, Paeth).
- Support for Truecolor, Truecolor with Alpha, Grayscale, and Indexed color types.