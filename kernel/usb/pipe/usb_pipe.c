#include "usb_pipe.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_pipe_t g_pipe_registry[MAX_GLOBAL_PIPES];
static bool g_pipe_used[MAX_GLOBAL_PIPES];
static atoms_spinlock_t g_pipe_lock;

void usb_pipe_manager_init(void) {
    memset(g_pipe_registry, 0, sizeof(g_pipe_registry));
    memset(g_pipe_used, 0, sizeof(g_pipe_used));
    atoms_spinlock_init(&g_pipe_lock, 1);
    display_print("[USB PIPE] Pipe Manager Subsystem Initialized.\n");
}

uint32_t usb_pipe_make_handle(uint8_t dev_addr, uint8_t ep_num, usb_direction_t dir, usb_transfer_type_t type) {
    return ((uint32_t)dev_addr << 16) | ((uint32_t)(ep_num & 0x0F) << 8) | ((uint32_t)(dir & 1) << 7) | ((uint32_t)type & 0x07);
}

usb_pipe_t* usb_create_pipe(usb_device_t* dev, uint8_t ep_num, usb_direction_t dir, usb_transfer_type_t type, uint16_t max_packet_size) {
    if (!dev) return NULL;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pipe_lock);
    
    for (uint32_t i = 0; i < MAX_GLOBAL_PIPES; i++) {
        if (!g_pipe_used[i]) {
            g_pipe_used[i] = true;
            usb_pipe_t* pipe = &g_pipe_registry[i];
            memset(pipe, 0, sizeof(usb_pipe_t));
            
            pipe->dev = dev;
            pipe->type = type;
            pipe->dir = dir;
            pipe->max_packet_size = (max_packet_size > 0) ? max_packet_size : 64;
            pipe->pipe_handle = usb_pipe_make_handle(dev->address, ep_num, dir, type);
            pipe->state = USB_PIPE_STATE_ACTIVE;
            pipe->ep = usb_endpoint_create(dev, ep_num, dir, type, max_packet_size, 0);
            atoms_spinlock_init(&pipe->lock, 1);
            
            atoms_spin_unlock_irqrestore(&g_pipe_lock, state);
            return pipe;
        }
    }
    
    atoms_spin_unlock_irqrestore(&g_pipe_lock, state);
    return NULL;
}

usb_pipe_t* usb_get_pipe_by_handle(uint32_t handle) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pipe_lock);
    for (uint32_t i = 0; i < MAX_GLOBAL_PIPES; i++) {
        if (g_pipe_used[i] && g_pipe_registry[i].pipe_handle == handle) {
            atoms_spin_unlock_irqrestore(&g_pipe_lock, state);
            return &g_pipe_registry[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_pipe_lock, state);
    return NULL;
}

bool usb_pipe_reset(usb_pipe_t* pipe) {
    if (!pipe) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pipe->lock);
    pipe->state = USB_PIPE_STATE_ACTIVE;
    if (pipe->ep) usb_endpoint_clear_halt(pipe->ep);
    atoms_spin_unlock_irqrestore(&pipe->lock, state);
    display_print("[USB PIPE] Pipe Reset Complete\n");
    return true;
}

bool usb_pipe_pause(usb_pipe_t* pipe) {
    if (!pipe) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pipe->lock);
    pipe->state = USB_PIPE_STATE_PAUSED;
    atoms_spin_unlock_irqrestore(&pipe->lock, state);
    return true;
}

bool usb_pipe_resume(usb_pipe_t* pipe) {
    if (!pipe) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pipe->lock);
    pipe->state = USB_PIPE_STATE_ACTIVE;
    atoms_spin_unlock_irqrestore(&pipe->lock, state);
    return true;
}

bool usb_pipe_shutdown(usb_pipe_t* pipe) {
    if (!pipe) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&pipe->lock);
    pipe->state = USB_PIPE_STATE_SHUTDOWN;
    atoms_spin_unlock_irqrestore(&pipe->lock, state);
    display_print("[USB PIPE] Pipe Shutdown Complete\n");
    return true;
}

void usb_pipe_free(usb_pipe_t* pipe) {
    if (!pipe) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pipe_lock);
    uint64_t idx = ((uint64_t)pipe - (uint64_t)g_pipe_registry) / sizeof(usb_pipe_t);
    if (idx < MAX_GLOBAL_PIPES) {
        g_pipe_used[idx] = false;
    }
    atoms_spin_unlock_irqrestore(&g_pipe_lock, state);
}
