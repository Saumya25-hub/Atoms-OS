#include "kernel/audio/drivers/ac97/ac97.h"
#include "kernel/audio/drivers/ac97/ac97_codec.h"
#include "kernel/audio/drivers/ac97/ac97_dma.h"
#include "kernel/audio/drivers/ac97/ac97_playback.h"
#include "kernel/audio/hal/audio_driver_registry.h"
#include "kernel/audio/hal/audio_hal.h"
#include "kernel/core/pci/pci.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/drivers/display/display.h"

static bool ac97_hal_init(void* device_info) {
    if (!device_info) return false;
    uint32_t* pci_info = (uint32_t*)device_info;
    uint8_t target_bus = (uint8_t)pci_info[0];
    uint8_t target_slot = (uint8_t)pci_info[1];
    
    // Read BAR0 (NAM) and BAR1 (NABM)
    uint32_t bar0 = pci_read_config_32(target_bus, target_slot, 0, 0x10);
    uint32_t bar1 = pci_read_config_32(target_bus, target_slot, 0, 0x14);
    
    if (!(bar0 & 1) || !(bar1 & 1)) {
        display_print("[AC97] FAILED: BARs are not I/O mapped.\n");
        return false;
    }
    
    uint16_t nam_bar = (uint16_t)(bar0 & ~3);
    uint16_t nabm_bar = (uint16_t)(bar1 & ~3);
    
    uint16_t cmd = pci_read_config_16(target_bus, target_slot, 0, 0x04);
    pci_write_config_16(target_bus, target_slot, 0, 0x04, cmd | 0x0005);
    
    ac97_codec_init_base(nam_bar, nabm_bar);
    
    if (!ac97_codec_verify_and_configure()) {
        return false;
    }
    
    ac97_dma_init(nabm_bar);
    return ac97_playback_init();
}

static void ac97_hal_shutdown(void) {
    ac97_playback_shutdown();
}

static bool ac97_hal_start_stream(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth) {
    (void)sample_rate;
    (void)channels;
    (void)bit_depth;
    ac97_playback_prepare();
    ac97_playback_start();
    return true;
}

static void ac97_hal_stop_stream(void) {
    ac97_playback_stop();
}

static void ac97_hal_pause_stream(void) {
    ac97_playback_stop();
}

static void ac97_hal_resume_stream(void) {
    ac97_playback_start();
}

static void ac97_hal_update_pointers(uint32_t frames_written) {
    (void)frames_written;
    ac97_playback_update();
}

static audio_hal_driver_t ac97_driver = {
    .name = "Intel AC97 Audio Controller",
    .capabilities = 0,
    .init = ac97_hal_init,
    .shutdown = ac97_hal_shutdown,
    .start_stream = ac97_hal_start_stream,
    .stop_stream = ac97_hal_stop_stream,
    .pause_stream = ac97_hal_pause_stream,
    .resume_stream = ac97_hal_resume_stream,
    .allocate_dma_buffer = NULL, // Handled internally by AC97 DMA for now
    .update_pointers = ac97_hal_update_pointers,
    .get_hardware_position = NULL,
    .set_hardware_volume = NULL,
    .register_irq_callback = NULL
};

void ac97_driver_register(void) {
    audio_hal_register_driver(&ac97_driver);
}
