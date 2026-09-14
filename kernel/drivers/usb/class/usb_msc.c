#include "usb_msc.h"
#include "kernel/drivers/usb/core/usb_registry.h"
#include "kernel/drivers/usb/host/xhci/xhci.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern void display_print(const char*);
extern void display_print_dec(uint64_t);
extern void display_print_hex(uint64_t);
extern void delay_cycles(uint64_t);

static usb_msc_device_t g_msc_devices[USB_MSC_MAX_DEVICES];
static uint32_t g_msc_device_count = 0;
static uint32_t g_msc_cbw_tag = 0x1000;

typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) USBEndpointDescriptor;

static inline uint64_t usb_msc_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static void usb_msc_delay_ms(uint32_t ms) {
    uint64_t total_cycles = (uint64_t)ms * 3500000ULL;
    uint64_t start = usb_msc_rdtsc();
    while ((usb_msc_rdtsc() - start) < total_cycles) {
        __asm__ volatile ("pause");
    }
}

uint32_t usb_msc_get_device_count(void) {
    return g_msc_device_count;
}

usb_msc_device_t* usb_msc_get_device(uint32_t index) {
    if (index >= g_msc_device_count) return NULL;
    return &g_msc_devices[index];
}

// Reset Bulk-Only Mass Storage Device
static bool usb_msc_reset(USBDevice* dev, uint8_t iface_num) {
    display_print("[MSC] Issuing Bulk-Only Mass Storage Reset...\n");
    bool ok = usb_control_transfer(dev,
                                  USB_REQ_TYPE_CLASS | USB_REQ_DIR_OUT | USB_REQ_REC_INTERFACE,
                                  UR_BBB_RESET, 0, iface_num, 0, NULL);
    if (!ok) {
        display_print("[MSC] Warning: Mass Storage Reset request rejected or failed\n");
    }
    return ok;
}

// Clear Endpoint Halt (STALL) condition
static bool usb_msc_clear_halt(USBDevice* dev, uint8_t ep_addr) {
    display_print("[MSC] Clearing Feature ENDPOINT_HALT on 0x");
    display_print_hex(ep_addr);
    display_print("\n");
    return usb_control_transfer(dev,
                                USB_REQ_TYPE_STANDARD | USB_REQ_DIR_OUT | USB_REQ_REC_ENDPOINT,
                                USB_REQ_CLEAR_FEATURE, 0 /* ENDPOINT_HALT */, ep_addr, 0, NULL);
}

// Query Maximum Logical Unit Number (GET_MAX_LUN)
static uint8_t usb_msc_get_max_lun(USBDevice* dev, uint8_t iface_num) {
    uint8_t max_lun = 0;
    bool ok = usb_control_transfer(dev,
                                  USB_REQ_TYPE_CLASS | USB_REQ_DIR_IN | USB_REQ_REC_INTERFACE,
                                  UR_BBB_GET_MAX_LUN, 0, iface_num, 1, &max_lun);
    if (!ok) {
        // Devices that do not support multiple LUNs may stall GET_MAX_LUN (per BOT 3.2.1)
        display_print("[MSC] GET_MAX_LUN stalled or unsupported; defaulting to LUN 0\n");
        return 0;
    }
    display_print("[MSC] Device reports Max LUN: ");
    display_print_dec(max_lun);
    display_print("\n");
    return max_lun;
}

// Execute Command over Bulk-Only Transport (CBW -> Data -> CSW)
static bool usb_msc_bot_execute(usb_msc_device_t* msc, const void* cdb, uint8_t cdb_len,
                                void* data, uint32_t data_len, bool dir_in) {
    if (!msc || !msc->usb_dev || !cdb || cdb_len == 0 || cdb_len > 16) {
        return false;
    }

    uint32_t current_tag = ++g_msc_cbw_tag;

    // 1. Prepare Command Block Wrapper (CBW)
    usb_msc_cbw_t cbw;
    memset(&cbw, 0, sizeof(cbw));
    cbw.dCBWSignature = CBWSIGNATURE;
    cbw.dCBWTag = current_tag;
    cbw.dCBWDataTransferLength = data_len;
    cbw.bmCBWFlags = dir_in ? CBWFLAGS_IN : CBWFLAGS_OUT;
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = cdb_len;
    memcpy(cbw.CBWCB, cdb, cdb_len);

    // 2. Send CBW via Bulk OUT
    uint32_t actual_bytes = 0;
    bool ok = usb_bulk_transfer(msc->usb_dev, msc->bulk_out_ep, &cbw, sizeof(cbw), &actual_bytes, 5000);
    if (!ok || actual_bytes != sizeof(cbw)) {
        display_print("[BOT] ❌ CBW Transfer Failed\n");
        usb_msc_clear_halt(msc->usb_dev, msc->bulk_out_ep);
        return false;
    }
    display_print("[BOT] CBW PASS Tag=");
    display_print_hex(current_tag);
    display_print(" Cmd=0x");
    display_print_hex(((const uint8_t*)cdb)[0]);
    display_print("\n");

    // 3. Data Stage (if applicable)
    if (data_len > 0 && data) {
        if (dir_in) {
            ok = usb_bulk_transfer(msc->usb_dev, msc->bulk_in_ep | 0x80, data, data_len, &actual_bytes, 5000);
        } else {
            ok = usb_bulk_transfer(msc->usb_dev, msc->bulk_out_ep, data, data_len, &actual_bytes, 5000);
        }

        if (!ok) {
            display_print("[BOT] ❌ DATA Stage Failed, attempted ");
            display_print_dec(data_len);
            display_print(" bytes\n");
            uint8_t stalled_ep = dir_in ? (msc->bulk_in_ep | 0x80) : msc->bulk_out_ep;
            usb_msc_clear_halt(msc->usb_dev, stalled_ep);
        } else {
            display_print("[BOT] DATA PASS Bytes=");
            display_print_dec(actual_bytes);
            display_print("\n");
        }
    }

    // 4. Receive Command Status Wrapper (CSW)
    usb_msc_csw_t csw;
    memset(&csw, 0, sizeof(csw));
    actual_bytes = 0;
    ok = usb_bulk_transfer(msc->usb_dev, msc->bulk_in_ep | 0x80, &csw, sizeof(csw), &actual_bytes, 5000);
    if (!ok) {
        display_print("[BOT] Warning: CSW transfer failed; clearing Bulk IN halt and retrying...\n");
        usb_msc_clear_halt(msc->usb_dev, msc->bulk_in_ep | 0x80);
        ok = usb_bulk_transfer(msc->usb_dev, msc->bulk_in_ep | 0x80, &csw, sizeof(csw), &actual_bytes, 5000);
        if (!ok) {
            display_print("[BOT] ❌ CSW Retry Failed\n");
            return false;
        }
    }

    // 5. Validate CSW
    if (csw.dCSWSignature != CSWSIGNATURE) {
        display_print("[BOT] ❌ CSW Signature Invalid: 0x");
        display_print_hex(csw.dCSWSignature);
        display_print("\n");
        return false;
    }

    if (csw.dCSWTag != current_tag) {
        display_print("[BOT] ❌ CSW Tag Mismatch: expected 0x");
        display_print_hex(current_tag);
        display_print(" got 0x");
        display_print_hex(csw.dCSWTag);
        display_print("\n");
        return false;
    }

    if (csw.bCSWStatus != CSWSTATUS_GOOD) {
        display_print("[BOT] ❌ CSW Status Failed: Code=");
        display_print_dec(csw.bCSWStatus);
        display_print(" Residue=");
        display_print_dec(csw.dCSWDataResidue);
        display_print("\n");
        if (csw.bCSWStatus == CSWSTATUS_PHASE) {
            usb_msc_reset(msc->usb_dev, msc->interface_number);
        }
        return false;
    }

    display_print("[BOT] CSW PASS Residue=");
    display_print_dec(csw.dCSWDataResidue);
    display_print(" Status=0\n");
    return true;
}

// --------------------------------------------------------------------------
// SCSI Command Set Implementations
// --------------------------------------------------------------------------

bool usb_msc_scsi_request_sense(usb_msc_device_t* msc) {
    if (!msc) return false;
    uint8_t cdb[6] = { SCSI_REQUEST_SENSE, 0, 0, 0, 18, 0 };
    scsi_request_sense_response_t sense;
    memset(&sense, 0, sizeof(sense));

    if (!usb_msc_bot_execute(msc, cdb, 6, &sense, 18, true)) {
        display_print("[SCSI] REQUEST SENSE Failed\n");
        return false;
    }

    display_print("[SCSI] REQUEST SENSE PASS: Key=0x");
    extern void display_print_hex(uint64_t);
    display_print_hex(sense.sense_key);
    display_print(" ASC=0x");
    display_print_hex(sense.additional_sense_code);
    display_print(" ASCQ=0x");
    display_print_hex(sense.additional_sense_code_qualifier);
    display_print("\n");
    return true;
}

bool usb_msc_scsi_test_unit_ready(usb_msc_device_t* msc) {
    if (!msc) return false;
    uint8_t cdb[6] = { SCSI_TEST_UNIT_READY, 0, 0, 0, 0, 0 };

    display_print("[SCSI] Testing Unit Ready (giving physical NAND up to 1.5s spin-up)...\n");
    for (int attempt = 1; attempt <= 15; attempt++) {
        if (usb_msc_bot_execute(msc, cdb, 6, NULL, 0, false)) {
            display_print("[SCSI] TEST UNIT READY PASS on attempt ");
            display_print_dec(attempt);
            display_print("\n");
            return true;
        }
        // If failed, issue REQUEST SENSE to clear UNIT ATTENTION condition on real flash drives
        usb_msc_scsi_request_sense(msc);
        usb_msc_delay_ms(100); // Genuine 100ms delay per attempt
    }
    display_print("[SCSI] TEST UNIT READY Failed after 15 attempts\n");
    return false;
}

bool usb_msc_scsi_inquiry(usb_msc_device_t* msc) {
    if (!msc) return false;
    uint8_t cdb[6] = { SCSI_INQUIRY, 0, 0, 0, 36, 0 };
    scsi_inquiry_response_t inq;
    memset(&inq, 0, sizeof(inq));

    if (!usb_msc_bot_execute(msc, cdb, 6, &inq, 36, true)) {
        display_print("[SCSI] ❌ INQUIRY Failed\n");
        return false;
    }

    memcpy(msc->vendor_id, inq.vendor_id, 8);
    msc->vendor_id[8] = '\0';
    memcpy(msc->product_id, inq.product_id, 16);
    msc->product_id[16] = '\0';
    memcpy(msc->product_rev, inq.product_rev, 4);
    msc->product_rev[4] = '\0';

    display_print("[SCSI] INQUIRY PASS: Vendor='");
    display_print(msc->vendor_id);
    display_print("' Product='");
    display_print(msc->product_id);
    display_print("' Rev='");
    display_print(msc->product_rev);
    display_print("'\n");

    return true;
}

bool usb_msc_scsi_read_capacity(usb_msc_device_t* msc) {
    if (!msc) return false;
    uint8_t cdb[10] = { SCSI_READ_CAPACITY_10, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    scsi_read_capacity_10_response_t cap;
    memset(&cap, 0, sizeof(cap));

    if (!usb_msc_bot_execute(msc, cdb, 10, &cap, sizeof(cap), true)) {
        display_print("[SCSI] ❌ READ CAPACITY Failed\n");
        return false;
    }

    // Convert Big-Endian to Native Little-Endian
    uint32_t last_lba = ((cap.last_lba >> 24) & 0xFF) |
                        ((cap.last_lba >> 8)  & 0xFF00) |
                        ((cap.last_lba << 8)  & 0xFF0000) |
                        ((cap.last_lba << 24) & 0xFF000000);

    uint32_t block_size = ((cap.block_size >> 24) & 0xFF) |
                          ((cap.block_size >> 8)  & 0xFF00) |
                          ((cap.block_size << 8)  & 0xFF0000) |
                          ((cap.block_size << 24) & 0xFF000000);

    if (block_size == 0) block_size = 512;

    msc->block_size = block_size;
    msc->total_blocks = (uint64_t)last_lba + 1;

    uint64_t size_mb = (msc->total_blocks * (uint64_t)msc->block_size) / (1024 * 1024);

    display_print("[SCSI] READ CAPACITY PASS: Blocks=");
    display_print_dec(msc->total_blocks);
    display_print(" BlockSize=");
    display_print_dec(msc->block_size);
    display_print(" Size=");
    display_print_dec(size_mb);
    display_print(" MB\n");

    return true;
}

bool usb_msc_scsi_read10(usb_msc_device_t* msc, uint64_t lba, uint32_t count, void* buffer) {
    if (!msc || !buffer || count == 0) return false;

    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_READ_10;
    cdb[2] = (lba >> 24) & 0xFF;
    cdb[3] = (lba >> 16) & 0xFF;
    cdb[4] = (lba >> 8)  & 0xFF;
    cdb[5] = lba & 0xFF;
    cdb[7] = (count >> 8) & 0xFF;
    cdb[8] = count & 0xFF;

    uint32_t transfer_bytes = count * msc->block_size;
    bool ok = usb_msc_bot_execute(msc, cdb, 10, buffer, transfer_bytes, true);
    if (!ok) {
        display_print("[SCSI] ❌ READ(10) Failed LBA=");
        display_print_dec(lba);
        display_print(" Count=");
        display_print_dec(count);
        display_print("\n");
        return false;
    }

    display_print("[SCSI] READ(10) PASS LBA=");
    display_print_dec(lba);
    display_print(" Count=");
    display_print_dec(count);
    display_print("\n");
    return true;
}

bool usb_msc_scsi_write10(usb_msc_device_t* msc, uint64_t lba, uint32_t count, const void* buffer) {
    if (!msc || !buffer || count == 0) return false;

    uint8_t cdb[10];
    memset(cdb, 0, sizeof(cdb));
    cdb[0] = SCSI_WRITE_10;
    cdb[2] = (lba >> 24) & 0xFF;
    cdb[3] = (lba >> 16) & 0xFF;
    cdb[4] = (lba >> 8)  & 0xFF;
    cdb[5] = lba & 0xFF;
    cdb[7] = (count >> 8) & 0xFF;
    cdb[8] = count & 0xFF;

    uint32_t transfer_bytes = count * msc->block_size;
    bool ok = usb_msc_bot_execute(msc, cdb, 10, (void*)buffer, transfer_bytes, false);
    if (!ok) {
        display_print("[SCSI] ❌ WRITE(10) Failed LBA=");
        display_print_dec(lba);
        display_print("\n");
        return false;
    }

    display_print("[SCSI] WRITE(10) PASS LBA=");
    display_print_dec(lba);
    display_print(" Count=");
    display_print_dec(count);
    display_print("\n");
    return true;
}

// --------------------------------------------------------------------------
// ATOMS BlockDevice Bridge Callbacks
// --------------------------------------------------------------------------

static bool usb_msc_bdev_read(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !dev->driver_data) return false;
    usb_msc_device_t* msc = (usb_msc_device_t*)dev->driver_data;
    return usb_msc_scsi_read10(msc, lba, count, buffer);
}

static bool usb_msc_bdev_write(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !dev->driver_data) return false;
    usb_msc_device_t* msc = (usb_msc_device_t*)dev->driver_data;
    return usb_msc_scsi_write10(msc, lba, count, buffer);
}

static bool usb_msc_bdev_flush(BlockDevice* dev) {
    (void)dev;
    return true;
}

// Register all discovered USB MSC devices with the kernel's active BlockDevice registry
void usb_msc_register_block_devices(void) {
    static char bdev_names[USB_MSC_MAX_DEVICES][16];

    for (uint32_t i = 0; i < g_msc_device_count; i++) {
        usb_msc_device_t* msc = &g_msc_devices[i];
        if (!msc->is_active || msc->block_device_id >= 0) continue;

        char* name = bdev_names[i];
        name[0] = 'u'; name[1] = 's'; name[2] = 'b';
        name[3] = '0' + (char)i;
        name[4] = '\0';

        msc->bdev.name = name;
        msc->bdev.sector_size = msc->block_size;
        msc->bdev.sector_count = msc->total_blocks;
        msc->bdev.read_only = false;
        msc->bdev.driver_data = msc;
        msc->bdev.read = usb_msc_bdev_read;
        msc->bdev.write = usb_msc_bdev_write;
        msc->bdev.flush = usb_msc_bdev_flush;

        extern int block_device_register(BlockDevice* device);
        msc->block_device_id = block_device_register(&msc->bdev);

        display_print("[BLOCK] usb");
        display_print_dec(i);
        display_print(" registered (Global Block ID: ");
        display_print_dec(msc->block_device_id);
        display_print(", Sectors: ");
        display_print_dec(msc->total_blocks);
        display_print(")\n");
    }
}

// --------------------------------------------------------------------------
// Class Driver Bind Handler
// --------------------------------------------------------------------------

static bool usb_msc_bind(USBDevice* dev, USBInterfaceDescriptor* interface_desc,
                         void* config_desc_buffer, uint16_t total_length) {
    if (!dev || !interface_desc || !config_desc_buffer) return false;

    display_print("[MSC] Mass Storage Class detected on Slot ");
    display_print_dec(dev->slot_id);
    display_print(" Interface ");
    display_print_dec(interface_desc->bInterfaceNumber);
    display_print(" (SubClass: 0x");
    display_print_hex(interface_desc->bInterfaceSubClass);
    display_print(" Protocol: 0x");
    display_print_hex(interface_desc->bInterfaceProtocol);
    display_print(")\n");

    if (g_msc_device_count >= USB_MSC_MAX_DEVICES) {
        display_print("[MSC] Error: Maximum USB MSC device limit reached\n");
        return false;
    }

    // Discover Bulk IN and Bulk OUT endpoints following the interface descriptor
    uint8_t* ptr = (uint8_t*)interface_desc + interface_desc->bLength;
    uint8_t* end = (uint8_t*)config_desc_buffer + total_length;

    uint8_t bulk_in_ep = 0;
    uint16_t bulk_in_max = 0;
    uint8_t bulk_out_ep = 0;
    uint16_t bulk_out_max = 0;

    while (ptr < end) {
        USBDescriptorHeader* hdr = (USBDescriptorHeader*)ptr;
        if (hdr->bLength == 0) break;
        if (hdr->bDescriptorType == USB_DESC_INTERFACE) break; // Next interface

        if (hdr->bDescriptorType == USB_DESC_ENDPOINT) {
            USBEndpointDescriptor* ep = (USBEndpointDescriptor*)ptr;
            if ((ep->bmAttributes & 0x03) == 0x02) { // Bulk endpoint
                if (ep->bEndpointAddress & 0x80) {
                    bulk_in_ep = ep->bEndpointAddress & 0x0F;
                    bulk_in_max = ep->wMaxPacketSize;
                    display_print("[MSC] Bulk IN endpoint = 0x");
                    display_print_hex(ep->bEndpointAddress);
                    display_print(" (MaxPacket = ");
                    display_print_dec(bulk_in_max);
                    display_print(")\n");
                } else {
                    bulk_out_ep = ep->bEndpointAddress & 0x0F;
                    bulk_out_max = ep->wMaxPacketSize;
                    display_print("[MSC] Bulk OUT endpoint = 0x");
                    display_print_hex(ep->bEndpointAddress);
                    display_print(" (MaxPacket = ");
                    display_print_dec(bulk_out_max);
                    display_print(")\n");
                }
            }
        }
        ptr += hdr->bLength;
    }

    if (bulk_in_ep == 0 || bulk_out_ep == 0) {
        display_print("[MSC] Error: Failed to find both Bulk IN and Bulk OUT endpoints\n");
        return false;
    }

    // Configure bulk endpoints on xHCI controller
    if (!xhci_configure_bulk_endpoints(dev, bulk_in_ep, bulk_in_max, bulk_out_ep, bulk_out_max)) {
        display_print("[MSC] Error: Failed to configure xHCI bulk endpoints\n");
        return false;
    }

    usb_msc_device_t* msc = &g_msc_devices[g_msc_device_count];
    memset(msc, 0, sizeof(usb_msc_device_t));
    msc->usb_dev = dev;
    msc->slot_id = dev->slot_id;
    msc->interface_number = interface_desc->bInterfaceNumber;
    msc->bulk_in_ep = bulk_in_ep;
    msc->bulk_in_max_packet = bulk_in_max;
    msc->bulk_out_ep = bulk_out_ep;
    msc->bulk_out_max_packet = bulk_out_max;
    msc->block_device_id = -1;

    // Reset BOT state and query LUNs
    usb_msc_reset(dev, interface_desc->bInterfaceNumber);
    msc->max_lun = usb_msc_get_max_lun(dev, interface_desc->bInterfaceNumber);

    // Give real physical drive 100ms after BOT reset before first SCSI command
    usb_msc_delay_ms(100);

    // Verify readiness and identify device
    usb_msc_scsi_test_unit_ready(msc);
    usb_msc_scsi_inquiry(msc);
    
    // Read Capacity (retry up to 3 times if initial attempt was busy)
    bool cap_ok = false;
    for (int c_try = 0; c_try < 3; c_try++) {
        if (usb_msc_scsi_read_capacity(msc)) {
            cap_ok = true;
            break;
        }
        usb_msc_delay_ms(100);
    }

    if (!cap_ok || msc->total_blocks == 0) {
        display_print("[MSC] Warning: Read capacity returned 0 blocks, defaulting to standard sector count\n");
        if (msc->block_size == 0) msc->block_size = 512;
    }

    msc->is_active = true;
    dev->driver_data = msc;
    g_msc_device_count++;

    display_print("[MSC] USB Mass Storage device initialization COMPLETE! Registered as Active MSC Device\n");
    return true;
}

// Register USB MSC Driver with ATOMS USB Core Registry
void usb_msc_init(void) {
    USBClassDriver msc_driver;
    memset(&msc_driver, 0, sizeof(msc_driver));
    msc_driver.name = "USB Mass Storage Class (BOT/SCSI)";
    msc_driver.class_code = 0x08;       // USB Mass Storage Class
    msc_driver.subclass_code = 0xFF;   // Match any subclass (0x06 SCSI Transparent, etc.)
    msc_driver.protocol_code = 0x50;   // Bulk-Only Transport (BOT)
    msc_driver.bind = usb_msc_bind;

    usb_register_class_driver(msc_driver);
    display_print("[USB] Registered USB Mass Storage Class Driver (BOT/SCSI)\n");
}
