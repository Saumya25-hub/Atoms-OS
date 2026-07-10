/**
 * @file agdpe.h
 * @brief ATOMEGear Display Platform Engine (AGDPE) Interface
 *
 * This provides the ultimate Hardware Abstraction Layer (HAL) for physical
 * graphics devices, supporting VBE and future GPU drivers seamlessly.
 */

#ifndef AGDPE_H
#define AGDPE_H

#include <stdint.h>
#include <stdbool.h>
#include "bovisual/Include/bovisual_types.h"

#define AGDPE_MAX_DISPLAYS 4

typedef enum {
    AGDPE_DISPLAY_STATE_DISCONNECTED = 0,
    AGDPE_DISPLAY_STATE_ACTIVE = 1,
    AGDPE_DISPLAY_STATE_SUSPENDED = 2
} AGDPE_DisplayState;

typedef enum {
    AGDPE_DRIVER_TYPE_VBE = 0,
    AGDPE_DRIVER_TYPE_BGA = 1,
    AGDPE_DRIVER_TYPE_VIRTIO = 2,
    AGDPE_DRIVER_TYPE_PCI_GPU = 3
} AGDPE_DriverType;

/**
 * @brief Represents a physical hardware display output.
 */
typedef struct AGDPE_DisplayDevice {
    uint32_t display_id;
    AGDPE_DisplayState state;
    AGDPE_DriverType driver_type;
    
    char name[32];
    
    /* Current Hardware Metrics */
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    
    /* Capabilities */
    bool supports_hardware_cursor;
    bool supports_vsync;
    bool supports_page_flipping;
    
    /* Device-specific internal handle */
    void* internal_handle;
    
    /* Public exposed memory (if accessible by CPU directly) */
    BVFramebuffer framebuffer;
    
    /* Interface */
    void (*SwapBuffers)(struct AGDPE_DisplayDevice* dev, const BVFramebuffer* backbuffer);
    bool (*SetMode)(struct AGDPE_DisplayDevice* dev, uint32_t width, uint32_t height, uint32_t bpp);
    void (*WaitForVSync)(struct AGDPE_DisplayDevice* dev);
} AGDPE_DisplayDevice;

/**
 * @brief Engine API
 */

/* Initializes the Platform Engine core */
void AGDPE_Initialize(void);

/* Registers a display device with the engine */
bool AGDPE_RegisterDevice(AGDPE_DisplayDevice* device);

/* Retrieves the total number of connected displays */
uint32_t AGDPE_GetActiveDisplayCount(void);

/* Retrieves the primary display device (ID 0) */
AGDPE_DisplayDevice* AGDPE_GetPrimaryDisplay(void);

/* Retrieves a display device by ID */
AGDPE_DisplayDevice* AGDPE_GetDisplay(uint32_t display_id);

/**
 * @brief Internal Driver APIs
 */
void AGDPE_VBE_Driver_Initialize(void* boot_info);

#endif // AGDPE_H
