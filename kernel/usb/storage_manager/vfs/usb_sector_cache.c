#include "../include/usb_disk.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

typedef struct {
    uint32_t disk_id;
    uint32_t lba;
    uint8_t  data[512];
    bool     is_valid;
    bool     is_dirty;
} usb_cache_entry_t;

static usb_cache_entry_t g_sector_cache[128];
static uint32_t g_cache_hits = 0;
static uint32_t g_cache_misses = 0;
static atoms_spinlock_t g_cache_lock;

void usb_sector_cache_init(void) {
    atoms_spinlock_init(&g_cache_lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_cache_lock);
    memset(g_sector_cache, 0, sizeof(g_sector_cache));
    g_cache_hits = 0;
    g_cache_misses = 0;
    atoms_spin_unlock_irqrestore(&g_cache_lock, state);
}

bool usb_sector_cache_read(usb_disk_t* disk, uint32_t lba, uint8_t* buffer) {
    if (!disk || !buffer) return false;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_cache_lock);
    for (int i = 0; i < 128; i++) {
        if (g_sector_cache[i].is_valid && g_sector_cache[i].disk_id == disk->disk_id && g_sector_cache[i].lba == lba) {
            memcpy(buffer, g_sector_cache[i].data, 512);
            g_cache_hits++;
            atoms_spin_unlock_irqrestore(&g_cache_lock, state);
            return true;
        }
    }
    g_cache_misses++;
    atoms_spin_unlock_irqrestore(&g_cache_lock, state);
    
    bool ok = usb_disk_read_sectors(disk, lba, 1, buffer);
    if (ok) {
        atoms_irq_lock_state_t state2 = atoms_spin_lock_irqsave(&g_cache_lock);
        uint32_t slot = lba % 128;
        g_sector_cache[slot].disk_id = disk->disk_id;
        g_sector_cache[slot].lba = lba;
        memcpy(g_sector_cache[slot].data, buffer, 512);
        g_sector_cache[slot].is_valid = true;
        g_sector_cache[slot].is_dirty = false;
        atoms_spin_unlock_irqrestore(&g_cache_lock, state2);
    }
    return ok;
}
