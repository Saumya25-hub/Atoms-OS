# Future Formats

The modular nature of BOPAWN makes it easy to add future formats (e.g., JPEG, GIF, WebP). To add a new format:
1. Create a decoder function.
2. Add its magic number to format detection.
3. Add it to the decoder registry.