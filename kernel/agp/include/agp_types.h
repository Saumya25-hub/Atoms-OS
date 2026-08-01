#ifndef AGP_TYPES_H
#define AGP_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AGP_MAX_SURFACES        64
#define AGP_MAX_CONTEXTS        64
#define AGP_MAX_TEXTURES        512
#define AGP_MAX_BUFFERS         512
#define AGP_MAX_SHADERS         256
#define AGP_MAX_PROGRAMS        128
#define AGP_MAX_COMMANDS        4096
#define AGP_MAX_UNIFORMS        64

typedef uint32_t AGPHandle;
typedef uint32_t AGPContextID;
typedef uint32_t AGPSurfaceID;
typedef uint32_t AGPTextureID;
typedef uint32_t AGPBufferID;
typedef uint32_t AGPShaderID;
typedef uint32_t AGPProgramID;

typedef enum {
    AGP_FORMAT_RGBA8888 = 1,
    AGP_FORMAT_RGB888   = 2,
    AGP_FORMAT_RGB565   = 3,
    AGP_FORMAT_DEPTH24  = 4
} AGPFormat;

typedef enum {
    AGP_BUFFER_VERTEX  = 1,
    AGP_BUFFER_INDEX   = 2,
    AGP_BUFFER_UNIFORM = 3
} AGPBufferType;

typedef enum {
    AGP_SHADER_VERTEX   = 1,
    AGP_SHADER_FRAGMENT = 2
} AGPShaderType;

typedef enum {
    AGP_PRIMITIVE_TRIANGLES = 1,
    AGP_PRIMITIVE_LINES     = 2,
    AGP_PRIMITIVE_POINTS    = 3,
    AGP_PRIMITIVE_QUADS     = 4
} AGPPrimitiveType;

typedef struct {
    float x, y, z;
    float r, g, b, a;
    float u, v;
} AGPVertex;

typedef struct {
    int32_t x, y, width, height;
} AGPRect;

typedef struct {
    uint32_t   width;
    uint32_t   height;
    AGPFormat  format;
    uint32_t*  pixels;
    bool       is_onscreen;
    uint32_t   window_id;
} AGPSurface;

typedef struct {
    AGPSurface back_buffers[3];
    uint32_t   buffer_count; // 2 or 3
    uint32_t   current_index;
    bool       vsync_enabled;
} AGPSwapchain;

typedef struct {
    uint32_t    context_id;
    AGPSurface* surface;
    AGPRect     viewport;
    
    // Matrix State
    float       modelview[16];
    float       projection[16];
    
    // Pipeline State
    uint32_t    clear_color;
    float       clear_depth;
    bool        depth_test;
    bool        blend_enabled;
    bool        cull_face;

    // Active Resources
    AGPTextureID active_texture;
    AGPBufferID  active_vbo;
    AGPBufferID  active_ebo;
    AGPProgramID active_program;

    // Direct Mode Immediate Buffer
    AGPVertex    imm_vertices[1024];
    uint32_t     imm_count;
    AGPPrimitiveType imm_primitive;
    bool         in_begin_end;

    uint32_t     ref_count;
} AGPContext;

typedef struct {
    uint32_t total_vram_bytes;
    uint32_t used_vram_bytes;
    uint32_t free_vram_bytes;
    uint32_t active_textures;
    uint32_t active_buffers;
    uint32_t draw_calls_sec;
    uint32_t frames_per_sec;
    uint32_t current_frame_ms;
} AGPDiagnostics;

#endif // AGP_TYPES_H
