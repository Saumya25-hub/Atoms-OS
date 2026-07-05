# Memory Model

Memory is allocated dynamically using kmalloc. The engine must carefully track buffers, especially during decompression, to avoid leaks. Once a BOSImage drops its reference count to zero, kfree is called on its underlying pixel data and surface structures.