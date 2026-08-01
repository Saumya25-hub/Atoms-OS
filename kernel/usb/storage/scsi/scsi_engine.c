#include "../include/usb_scsi.h"
#include "../include/usb_bot.h"
#include "../include/usb_storage_debug.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern void usb_bot_init(void);

static bool g_scsi_initialized = false;

void usb_storage_engine_init(void) {
    if (g_scsi_initialized) return;
    display_print("[UMS] Initializing USB Mass Storage & SCSI Engine...\n");
    usb_bot_init();
    ums_telemetry_init();
    g_scsi_initialized = true;
    display_print("[UMS] Subsystem Initialized. BOT & SCSI Engine Ready.\n");
}

usb_storage_device_t* usb_storage_device_create(uint32_t usb_device_id, uint8_t bulk_in_ep, uint8_t bulk_out_ep) {
    static usb_storage_device_t pool[8];
    static uint32_t count = 0;
    static atoms_spinlock_t lock;
    
    atoms_spinlock_init(&lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&lock);
    if (count >= 8) {
        atoms_spin_unlock_irqrestore(&lock, state);
        return NULL;
    }
    
    usb_storage_device_t* dev = &pool[count++];
    memset(dev, 0, sizeof(usb_storage_device_t));
    atoms_spinlock_init(&dev->lock, 0);
    dev->storage_id = count;
    dev->usb_device_id = usb_device_id;
    dev->bulk_in_ep = bulk_in_ep;
    dev->bulk_out_ep = bulk_out_ep;
    dev->block_size_bytes = 512;
    dev->total_lbas = 2097152; // 1GB default (2,097,152 * 512B)
    dev->is_ready = true;
    
    usb_device_t* cdev = usb_find_device(usb_device_id);
    usb_pipe_t* p_in = usb_create_pipe(cdev, bulk_in_ep, USB_DIR_IN, USB_TRANSFER_TYPE_BULK, 512);
    usb_pipe_t* p_out = usb_create_pipe(cdev, bulk_out_ep, USB_DIR_OUT, USB_TRANSFER_TYPE_BULK, 512);
    dev->bulk_in_pipe = p_in ? p_in->pipe_handle : 0x101;
    dev->bulk_out_pipe = p_out ? p_out->pipe_handle : 0x102;
    
    atoms_spin_unlock_irqrestore(&lock, state);
    return dev;
}

bool usb_storage_read_sectors(usb_storage_device_t* dev, uint32_t lba, uint16_t count, uint8_t* buffer) {
    if (!dev || !buffer || count == 0) return false;
    
    uint8_t cdb[10];
    scsi_build_read10_cdb(cdb, lba, count);
    
    usb_csw_t csw;
    uint32_t transfer_bytes = count * dev->block_size_bytes;
    return usb_bot_execute(dev, 0, cdb, 10, buffer, transfer_bytes, CBW_FLAGS_DATA_IN, &csw);
}

bool usb_storage_write_sectors(usb_storage_device_t* dev, uint32_t lba, uint16_t count, const uint8_t* buffer) {
    if (!dev || !buffer || count == 0 || dev->is_write_protected) return false;
    
    uint8_t cdb[10];
    scsi_build_write10_cdb(cdb, lba, count);
    
    usb_csw_t csw;
    uint32_t transfer_bytes = count * dev->block_size_bytes;
    return usb_bot_execute(dev, 0, cdb, 10, (uint8_t*)buffer, transfer_bytes, CBW_FLAGS_DATA_OUT, &csw);
}
