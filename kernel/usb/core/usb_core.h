#ifndef SIGNATURES_USB_CORE_H
#define SIGNATURES_USB_CORE_H

#include "../common/usb_common.h"
#include "kernel/core/sync/spinlock.h"

#define MAX_USB_DRIVERS 16
#define MAX_USB_DEVICES 32
#define MAX_USB_CONFIGURATIONS 4
#define MAX_USB_INTERFACES 8

// USB Device States (Standard USB Device State Machine)
typedef enum {
    USB_DEV_STATE_ATTACHED = 0,
    USB_DEV_STATE_POWERED,
    USB_DEV_STATE_DEFAULT,
    USB_DEV_STATE_ADDRESSED,
    USB_DEV_STATE_CONFIGURED,
    USB_DEV_STATE_SUSPENDED,
    USB_DEV_STATE_RESUMED,
    USB_DEV_STATE_DISCONNECTED,
    USB_DEV_STATE_RECOVERY,
    USB_DEV_STATE_ERROR
} usb_device_state_t;

// Forward declarations
struct usb_device;
struct usb_driver;
struct usb_urb;

typedef struct usb_driver {
    const char* name;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t dev_class;
    uint8_t dev_subclass;
    uint8_t dev_protocol;
    
    bool (*probe)(struct usb_device* dev);
    void (*disconnect)(struct usb_device* dev);
    void (*suspend)(struct usb_device* dev);
    void (*resume)(struct usb_device* dev);
} usb_driver_t;

typedef struct usb_device {
    uint32_t device_id;
    uint8_t address;
    uint8_t port_num;
    usb_speed_t speed;
    usb_device_state_t state;
    
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t dev_class;
    uint8_t dev_subclass;
    uint8_t dev_protocol;
    uint16_t max_packet_size0;
    
    uint32_t controller_id;
    usb_controller_type_t controller_type;
    void* controller_ctx;
    
    struct usb_driver* driver;
    void* driver_data;
    
    uint32_t active_config;
    uint32_t active_interface;
    uint32_t ref_count;
    atoms_spinlock_t lock;
} usb_device_t;

typedef struct {
    usb_driver_t drivers[MAX_USB_DRIVERS];
    uint32_t driver_count;
    
    usb_device_t devices[MAX_USB_DEVICES];
    uint32_t device_count;
    uint32_t next_address;
    
    atoms_spinlock_t lock;
    bool initialized;
} usb_core_registry_t;

// Public USB Core APIs
void usb_core_init(void);
bool usb_register_driver(usb_driver_t* driver);
bool usb_unregister_driver(usb_driver_t* driver);
usb_device_t* usb_register_device(uint32_t controller_id, usb_controller_type_t ctrl_type, uint8_t port, usb_speed_t speed);
bool usb_unregister_device(usb_device_t* dev);

usb_device_t* usb_find_device(uint32_t device_id);
usb_device_t* usb_get_device_by_address(uint8_t address);
usb_device_t* usb_get_device_by_slot(uint8_t slot_id);
void usb_get_device(usb_device_t* dev);
void usb_put_device(usb_device_t* dev);

bool usb_set_configuration(usb_device_t* dev, uint8_t config_val);
uint8_t usb_get_configuration(usb_device_t* dev);
bool usb_set_interface(usb_device_t* dev, uint8_t ifnum, uint8_t altsetting);
uint8_t usb_get_interface(usb_device_t* dev);

bool usb_reset_device(usb_device_t* dev);
bool usb_suspend_device(usb_device_t* dev);
bool usb_resume_device(usb_device_t* dev);
void usb_disconnect_device(usb_device_t* dev);

usb_core_registry_t* usb_get_core_registry(void);

#endif // SIGNATURES_USB_CORE_H
