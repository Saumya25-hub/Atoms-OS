# Walkthrough

When opawn_load(path) is called:
1. The cache is checked.
2. If missed, image_loader.c reads the file via VFS.
3. opawn.c detects the format.
4. The respective decoder (e.g., png_decoder.c) processes the bytes.
5. The resulting surface is cached and returned.