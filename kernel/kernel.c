void kernel_main() {
    // Pointer to VGA text buffer
    volatile unsigned short* vga_buffer = (volatile unsigned short*)0xB8000;
    
    // The message we want to print
    const char* str = "Kernel OK";
    
    // Start printing at Row 3 (160 * 3 = 480 bytes = 240 shorts)
    // This ensures all 4 messages are visible at once.
    int index = 240; 
    
    // 0x0F is White text on Black background.
    for (int i = 0; str[i] != '\0'; i++) {
        vga_buffer[index++] = (unsigned short)str[i] | (0x0F << 8);
    }
}
