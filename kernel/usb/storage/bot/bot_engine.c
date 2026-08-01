#include "../include/usb_bot.h"
#include "../include/usb_storage_debug.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern bool usb_bot_transport_stage(usb_storage_device_t* dev, usb_cbw_t* cbw, uint8_t* data_buf, uint32_t data_len, usb_csw_t* csw);

static usb_bot_engine_t g_bot_engine;

void usb_bot_init(void) {
    atoms_spinlock_init(&g_bot_engine.lock, 0);
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_bot_engine.lock);
    memset(&g_bot_engine, 0, sizeof(usb_bot_engine_t));
    g_bot_engine.state = BOT_STATE_IDLE;
    g_bot_engine.current_tag = 1000;
    atoms_spin_unlock_irqrestore(&g_bot_engine.lock, state);
    display_print("[BOT ENGINE] Bulk-Only Transport Engine Initialized.\n");
}

bool usb_bot_execute(usb_storage_device_t* dev, uint8_t lun, const uint8_t* cdb, uint8_t cdb_len, uint8_t* data_buf, uint32_t data_len, uint8_t dir_flags, usb_csw_t* out_csw) {
    if (!dev || !cdb || !out_csw) return false;
    
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_bot_engine.lock);
    uint32_t tag = g_bot_engine.current_tag++;
    g_bot_engine.total_cbws_sent++;
    g_bot_engine.state = BOT_STATE_CBW_SENT;
    atoms_spin_unlock_irqrestore(&g_bot_engine.lock, state);
    
    usb_cbw_t cbw;
    usb_cbw_init(&cbw, tag, data_len, dir_flags, lun, cdb_len, cdb);
    
    bool ok = usb_bot_transport_stage(dev, &cbw, data_buf, data_len, out_csw);
    
    atoms_irq_lock_state_t state2 = atoms_spin_lock_irqsave(&g_bot_engine.lock);
    if (ok && out_csw->bCSWStatus == CSW_STATUS_PASSED) {
        g_bot_engine.total_csws_received++;
        g_bot_engine.state = BOT_STATE_IDLE;
    } else {
        g_bot_engine.csw_failures++;
        g_bot_engine.state = BOT_STATE_ERROR;
    }
    atoms_spin_unlock_irqrestore(&g_bot_engine.lock, state2);
    
    return ok;
}
