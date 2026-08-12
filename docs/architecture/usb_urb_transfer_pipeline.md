# ATOMS OS — Universal USB Core & Transfer Engine (URB Pipeline) Specification

## 1. Executive Overview

The **Universal USB Core & Transfer Engine** models the Linux Kernel `usbcore` URB (USB Request Block) architecture. It abstracts physical USB Host Controller hardware (xHCI) behind an asynchronous, request-driven pipe model. 

By utilizing URBs with asynchronous completion callbacks, ATOMS OS guarantees that USB Control Requests (`GET_DESCRIPTOR`, `SET_PROTOCOL`) and USB Interrupt IN transfers (Mouse & Keyboard polling) NEVER block the kernel main thread, freeze the scheduler, or stall hardware interrupt handling.

---

## 2. URB (USB Request Block) Lifecycle State Machine

```
      +-------------------+
      |  UNINITIALIZED    |
      +-------------------+
                |
                v  usb_alloc_urb()
      +-------------------+
      |    SUBMITTED      | <---------------------+
      +-------------------+                       |
                |                                 |
                |  usb_submit_urb()               | Requeue (Continuous
                v                                 | Interrupt Polling)
+-----------------------------------+             |
| xHCI Transfer Ring Enqueue (TRB)  |             |
| Doorbell Ring -> Controller DMA   |             |
+-----------------------------------+             |
                |                                 |
                v  xHCI Transfer Event TRB        |
      +-------------------+                       |
      |    COMPLETED      | ----------------------+
      +-------------------+
                |
                v  urb->complete_callback(urb)
      +-------------------+
      |   CONSUMED / FREE |
      +-------------------+
```

---

## 3. URB Structure Definition

```c
typedef struct USBRequestBlock {
    struct USBDevice* dev;
    uint8_t ep_address;        // Bit 7: Direction (1=IN, 0=OUT), Bits [3:0]: EP Index
    uint8_t ep_type;           // 0=Control, 1=Isoc, 2=Bulk, 3=Interrupt
    uint8_t request_type;      // For Control transfers
    uint8_t request;           // For Control transfers
    uint16_t value;            // For Control transfers
    uint16_t index;            // For Control transfers
    void* transfer_buffer;     // Virtual buffer address
    uint32_t transfer_length;  // Requested transfer byte count
    uint32_t actual_length;    // Transferred byte count returned by xHCI
    int status;                // 0 = Success, -ETIMEDOUT, -EIO, -EINPROGRESS
    void (*complete_callback)(struct USBRequestBlock* urb);
    void* context;             // Class driver private context pointer
} USBRequestBlock;
```

---

## 4. xHCI Transfer Ring Mapping & Interrupt Handling

1. **Control Transfers**:
   - Mapped to EP0 (Slot Default Control Pipe).
   - Setup Stage TRB (`TRB_SETUP_STAGE`) -> Data Stage TRB (`TRB_DATA_STAGE`) -> Status Stage TRB (`TRB_STATUS_STAGE`).
   - Doorbell `Target = 1`.

2. **Interrupt IN Transfers**:
   - Mapped to Endpoint DCI (`(ep_num * 2) + 1`).
   - Configured via `TRB_CONFIGURE_ENDPOINT_CMD`.
   - Normal TRB (`TRB_NORMAL`) queued with `IOC` (Interrupt On Completion) bit set.
   - Upon Transfer Event TRB, `actual_length = transfer_length - residual`.
   - Callback executed; if continuous mode is enabled, URB is requeued instantly in the Event Ring handler loop.
