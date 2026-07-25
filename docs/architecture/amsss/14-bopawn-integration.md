# BOPAWN Integration

Default AMSSS registration installs a provider for the fixed 32-slot BOPAWN image cache. Its callback evicts only zero-reference images and destroys their surface and image through existing lifecycle functions. Reported bytes currently cover the image object; framebuffer ownership stays with the surface subsystem.

