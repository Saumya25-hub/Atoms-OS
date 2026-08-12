#ifndef ATOMS_USB_FORENSIC_CENTER_H
#define ATOMS_USB_FORENSIC_CENTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define FORENSIC_EVENT_RING_CAPACITY 64
#define FORENSIC_MAX_PORTS 16
#define FORENSIC_MAX_DEVICES 8

// Panel 1 — Host Controller Dashboard
typedef struct {
    char controller_type[16];
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint32_t usbcmd;
    uint32_t usbsts;
    uint32_t dnctrl;
    uint32_t config;
    uint64_t irq_count;
    char state[16];
} ForensicControllerDashboard;

// Panel 2 — Port Forensics
typedef struct {
    uint8_t port_num;
    bool ccs; // Current Connect Status
    bool ped; // Port Enabled/Disabled
    bool pr;  // Port Reset
    char speed[16];
    uint8_t slot_id;
    char status[16];
} ForensicPortStatus;

typedef struct {
    uint32_t port_count;
    ForensicPortStatus ports[FORENSIC_MAX_PORTS];
} ForensicPortPanel;

// Panel 3 — Event Ring Analyzer
typedef struct {
    uint64_t event_id;
    char type_str[32];
    uint32_t type;
    uint32_t completion_code;
    const char* completion_str;
    uint8_t slot_id;
    uint8_t endpoint_id;
    uint64_t trb_ptr;
    uint32_t raw_status;
    uint32_t raw_control;
} ForensicEventEntry;

typedef struct {
    ForensicEventEntry entries[FORENSIC_EVENT_RING_CAPACITY];
    uint32_t head;
    uint32_t tail;
    uint64_t total_events;
} ForensicEventRingPanel;

// Panel 4 — Control Transfer Analyzer
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    char direction[8];
    uint32_t setup_dw0, setup_dw1, setup_dw2, setup_dw3;
    uint32_t data_dw0, data_dw1, data_dw2, data_dw3;
    uint32_t status_dw0, status_dw1, status_dw2, status_dw3;
} ForensicControlTransferPanel;

// Panel 5 — DMA Analyzer
typedef struct {
    uint64_t dma_phys;
    uint64_t dma_virt;
    uint32_t length;
    bool cache_flush_pass;
    bool cache_invalidate_pass;
    uint8_t raw_dma[64];
} ForensicDmaPanel;

// Panel 6 — Interrupt Analyzer
typedef struct {
    uint64_t irq_count;
    uint64_t last_irq_timestamp;
    char last_event_type[32];
} ForensicInterruptPanel;

// Panel 7 — Enumeration Timeline
typedef struct {
    char stage_names[20][32];
    bool stage_passed[20];
    uint32_t current_stage;
    uint32_t completion_code;
    uint32_t residual_length;
    uint32_t retry_count;
} ForensicTimelinePanel;

// Panel 8 — USB Device Tree
typedef struct {
    uint8_t port_num;
    char device_name[32];
    uint16_t vid;
    uint16_t pid;
    char speed[16];
    char driver_name[16];
} ForensicUsbDevice;

typedef struct {
    uint32_t device_count;
    ForensicUsbDevice devices[FORENSIC_MAX_DEVICES];
} ForensicDeviceTreePanel;

// Panel 9 — Controller Comparison Layer
typedef struct {
    bool xhci_detected;
    bool ehci_detected;
    bool uhci_detected;
    bool ohci_detected;
    char controller_used[16];
    char fallback_mode[32];
    char failure_reason[64];
} ForensicComparisonPanel;

// Panel 10 — Hardware Snapshot
typedef struct {
    char motherboard[64];
    char cpu[64];
    char vendor_name[32];
    uint16_t vendor_id;
    uint16_t device_id;
    uint64_t bar0_phys;
    uint8_t irq_line;
    bool msi_enabled;
} ForensicHardwareSnapshot;

// Master Forensic Command Center Container
typedef struct {
    bool initialized;
    ForensicControllerDashboard dashboard;
    ForensicPortPanel           ports;
    ForensicEventRingPanel      event_ring;
    ForensicControlTransferPanel control_transfer;
    ForensicDmaPanel            dma;
    ForensicInterruptPanel      interrupts;
    ForensicTimelinePanel       timeline;
    ForensicDeviceTreePanel     tree;
    ForensicComparisonPanel     comparison;
    ForensicHardwareSnapshot    hardware;
} USBForensicCommandCenter;

extern USBForensicCommandCenter g_forensic_center;

// Core Public APIs
void usb_forensic_center_init(void);
void usb_forensic_center_update(void);
void usb_forensic_center_render(void);

void usb_forensic_log_event(uint32_t type, uint32_t code, uint8_t slot, uint8_t ep, uint64_t trb_ptr, uint32_t status, uint32_t control);
void usb_forensic_log_control_transfer(uint8_t req_type, uint8_t req, uint16_t val, uint16_t idx, uint16_t len);
void usb_forensic_log_dma(uint64_t phys, uint64_t virt, uint32_t len, const void* data);
void usb_forensic_log_stage(uint32_t stage, bool success, uint32_t code, uint32_t residual);

#endif // ATOMS_USB_FORENSIC_CENTER_H
