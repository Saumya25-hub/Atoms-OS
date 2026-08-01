#ifndef SIGNATURES_USB_BOT_H
#define SIGNATURES_USB_BOT_H

#include "../../common/usb_common.h"
#include "usb_cbw.h"
#include "usb_csw.h"
#include "usb_storage_device.h"

typedef enum {
    BOT_STATE_IDLE = 0,
    BOT_STATE_CBW_SENT,
    BOT_STATE_DATA_XFER,
    BOT_STATE_CSW_RECV,
    BOT_STATE_STALL_RECOVERY,
    BOT_STATE_ERROR
} bot_state_t;

typedef struct {
    bot_state_t      state;
    uint32_t         current_tag;
    uint32_t         total_cbws_sent;
    uint32_t         total_csws_received;
    uint32_t         csw_failures;
    uint32_t         phase_errors;
    atoms_spinlock_t lock;
} usb_bot_engine_t;

// API
void usb_bot_init(void);
bool usb_bot_execute(usb_storage_device_t* dev, uint8_t lun, const uint8_t* cdb, uint8_t cdb_len, uint8_t* data_buf, uint32_t data_len, uint8_t dir_flags, usb_csw_t* out_csw);
bool usb_bot_reset_recovery(usb_storage_device_t* dev);

#endif // SIGNATURES_USB_BOT_H
