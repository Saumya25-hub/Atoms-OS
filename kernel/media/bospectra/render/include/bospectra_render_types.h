#ifndef BOSPECTRA_RENDER_TYPES_H
#define BOSPECTRA_RENDER_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOSPECTRA_RENDER_BACKEND_SOFTWARE = 0, // CPU BOImage / BOTexture Blitter
    BOSPECTRA_RENDER_BACKEND_OPENGL,       // OpenGL Texture Upload Engine
    BOSPECTRA_RENDER_BACKEND_VULKAN        // Future Vulkan Engine
} bospectra_render_backend_t;

typedef enum {
    BOSPECTRA_SCALING_FILTER_NEAREST = 0,
    BOSPECTRA_SCALING_FILTER_BILINEAR
} bospectra_scaling_filter_t;

typedef enum {
    BOSPECTRA_ROTATION_0 = 0,
    BOSPECTRA_ROTATION_90 = 90,
    BOSPECTRA_ROTATION_180 = 180,
    BOSPECTRA_ROTATION_270 = 270
} bospectra_rotation_angle_t;

#define BOSPECTRA_RENDER_FLAG_FULLSCREEN    0x0001U
#define BOSPECTRA_RENDER_FLAG_KEEP_ASPECT   0x0002U
#define BOSPECTRA_RENDER_FLAG_VSYNC         0x0004U

typedef struct {
    bospectra_render_backend_t backend;
    bospectra_scaling_filter_t filter;
    bospectra_rotation_angle_t rotation;
    uint32_t                   target_window_id;
    uint32_t                   flags;
} BOSPECTRA_RenderSpec;

#endif // BOSPECTRA_RENDER_TYPES_H
