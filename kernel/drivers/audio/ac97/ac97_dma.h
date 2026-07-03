#ifndef AC97_DMA_H
#define AC97_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ac97_bdl.h"

typedef enum {
    AC97_DMA_STATE_UNINITIALIZED,
    AC97_DMA_STATE_ALLOCATED,
    AC97_DMA_STATE_PREPARED,
    AC97_DMA_STATE_READY,
    AC97_DMA_STATE_RUNNING,
    AC97_DMA_STATE_STOPPED,
    AC97_DMA_STATE_ERROR
} Ac97DmaState;

typedef struct {
    Ac97BdlEntry* bdl;                 // Virtual address of BDL
    uint32_t bdl_phys;                 // Physical address of BDL
    
    uint8_t* pcm_buffer;               // Virtual address of PCM buffer
    uint32_t pcm_buffer_phys;          // Physical address of PCM buffer
    size_t pcm_buffer_size;            // Total size of buffer in bytes
    
    Ac97DmaState state;
} Ac97DmaManager;

bool ac97_dma_init(uint16_t nabm_bar);
void ac97_dma_shutdown(void);
bool ac97_dma_reset(void);
bool ac97_dma_prepare(size_t buffer_bytes);
void ac97_dma_status(void);

Ac97DmaManager* ac97_dma_get_manager(void);
uint16_t ac97_dma_get_nabm_bar(void);

void ac97_dma_run_stress_test(void);

#endif // AC97_DMA_H
