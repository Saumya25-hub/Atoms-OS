import os

filepath = 'kernel/audio/drivers/ac97/ac97_dma.c'
with open(filepath, 'r') as f:
    data = f.read()

old = '''    for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
        g_dma.bdl[i].buffer_phys_addr = g_dma.pcm_buffer_phys + (i * bytes_per_entry);
        g_dma.bdl[i].length = bytes_per_entry / 2;
        g_dma.bdl[i].flags = 0; // Polling mode: No IOC interrupt to prevent level-triggered PCI IRQ storm
    }
    
    g_dma.state = AC97_DMA_STATE_PREPARED;'''

new = '''    for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
        g_dma.bdl[i].buffer_phys_addr = g_dma.pcm_buffer_phys + (i * bytes_per_entry);
        g_dma.bdl[i].length = bytes_per_entry / 2;
        g_dma.bdl[i].flags = 0; // Polling mode: No IOC interrupt to prevent level-triggered PCI IRQ storm
    }
    
    display_print("\\n[AC97 DMA] ===== PHYSICAL CONTIGUITY AUDIT =====\\n");
    uint32_t mismatches = 0;
    for (int i = 0; i < AC97_BDL_ENTRIES; i++) {
        uint32_t assumed_phys = g_dma.pcm_buffer_phys + (i * bytes_per_entry);
        uint32_t real_phys = get_phys(g_dma.pcm_buffer + (i * bytes_per_entry));
        if (assumed_phys != real_phys) {
            display_print("[DMA PHYSICAL MAPPING MISMATCH]\\n");
            display_print("descriptor: "); display_print_dec(i); display_print("\\n");
            display_print("expected_phys: "); display_print_hex(assumed_phys); display_print("\\n");
            display_print("actual_phys: "); display_print_hex(real_phys); display_print("\\n");
            mismatches++;
        }
    }
    
    if (mismatches > 0) {
        display_print("SMOKING GUN: DMA Buffer is NOT physically contiguous!\\n");
    } else {
        display_print("DMA Buffer is fully physically contiguous.\\n");
    }
    display_print("[AC97 DMA] ===== END AUDIT =====\\n\\n");

    g_dma.state = AC97_DMA_STATE_PREPARED;'''

data = data.replace(old, new)

with open(filepath, 'w') as f:
    f.write(data)

print("Patch applied.")
