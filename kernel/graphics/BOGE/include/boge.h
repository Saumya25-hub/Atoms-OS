#ifndef ATOMS_OS_BOGE_H
#define ATOMS_OS_BOGE_H

/**
 * @file boge.h
 * @brief BOS Graphics Engine V2 (BOGE V2) Public API Contract
 * @status Phase 0 Architecture Frozen
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Constant Definitions --- */
#define BOGE_MAX_SURFACES         256
#define BOGE_MAX_SURFACE_DAMAGE   16
#define BOGE_SURFACE_OPAQUE       (1 << 0)
#define BOGE_SURFACE_TRANSPARENT  (1 << 1)
#define BOGE_SURFACE_HIDDEN       (1 << 2)

/* --- Core Data Structures --- */
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} BOGE_Rect;

typedef struct {
    int16_t y;
    int16_t x1;
    int16_t x2;
} BOGE_Span;

typedef enum {
    BOGE_CMD_DRAW_LINE = 1,
    BOGE_CMD_FILL_RECT,
    BOGE_CMD_BLIT_BITMAP,
    BOGE_CMD_DRAW_GLYPH_STRING
} BOGE_CommandType;

typedef struct {
    BOGE_CommandType type;
    uint32_t color;
    int16_t x1, y1, x2, y2;
    union {
        struct { uint32_t resource_id; int16_t src_x, src_y; } blit;
        struct { uint32_t font_id; char text[32]; } draw_text;
    } data;
} BOGE_DrawCommand;

typedef struct {
    uint32_t frame_id;
    void* buffer_virtual_address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    BOGE_Rect dirty_rects[32];
    uint32_t dirty_count;
} BOGE_StagingFrame;

/* --- Core Lifecycle APIs --- */
bool BOGE_Initialize(void);
void BOGE_Shutdown(void);

/* --- Surface Management APIs --- */
uint32_t BOGE_Surface_Create(uint32_t width, uint32_t height, uint32_t flags);
void BOGE_Surface_Destroy(uint32_t surface_id);
bool BOGE_Surface_SetBounds(uint32_t surface_id, int32_t x, int32_t y, uint32_t width, uint32_t height);
bool BOGE_Surface_SetOpacity(uint32_t surface_id, uint8_t opacity);
void* BOGE_Surface_LockBuffer(uint32_t surface_id, uint32_t* out_pitch);
void BOGE_Surface_UnlockBufferAndInvalidate(uint32_t surface_id, BOGE_Rect dirty_rect);

/* --- Command Queue & Compositing APIs --- */
bool BOGE_SubmitDrawCommand(uint32_t surface_id, const BOGE_DrawCommand* cmd);
BOGE_StagingFrame* BOGE_ComposeFrame(void);

/* --- Resource Cache APIs --- */
uint32_t BOGE_Resource_CreateBitmap(uint32_t width, uint32_t height, const void* initial_data);
void BOGE_Resource_Release(uint32_t resource_id);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BOGE_H
