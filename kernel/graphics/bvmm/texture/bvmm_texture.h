#ifndef _BOS_BVMM_TEXTURE_H_
#define _BOS_BVMM_TEXTURE_H_

#include "../surface/bvmm_surface.h"
#include "kernel/core/sync/spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file bvmm_texture.h
 * @brief Production Texture & Format Engine (BTFE V1.0) Master Header
 * 
 * BTFE is the permanent GPU texture subsystem of BOS. It manages every GPU
 * texture object, format conversion, swizzle matrix, mipchain packing, tile layout,
 * texture view, and sampler descriptor in the system.
 */

#define BTFE_TEXTURE_CANARY_MAGIC   0x54455854  /* "TEXT" */
#define BTFE_MAX_TEXTURES           8192
#define BTFE_INVALID_TEXTURE_ID     0ULL
#define BTFE_MAX_MIP_LEVELS         16

typedef uint64_t btfe_texture_id_t;
typedef uint32_t btfe_sampler_id_t;

/* Production Format Descriptors (Phase 5C Spec) */
typedef enum {
    BTFE_FMT_RGBA8888       = 0,
    BTFE_FMT_BGRA8888       = 1,
    BTFE_FMT_ARGB8888       = 2,
    BTFE_FMT_ABGR8888       = 3,
    BTFE_FMT_RGB565         = 4,
    BTFE_FMT_R8             = 5,
    BTFE_FMT_RG8            = 6,
    BTFE_FMT_RGB8           = 7,
    BTFE_FMT_RGBA16         = 8,
    BTFE_FMT_RGBA16F        = 9,
    BTFE_FMT_RGBA32F        = 10,
    BTFE_FMT_DEPTH16        = 11,
    BTFE_FMT_DEPTH24        = 12,
    BTFE_FMT_DEPTH32        = 13,
    BTFE_FMT_STENCIL8       = 14,
    BTFE_FMT_DEPTH24STENCIL8 = 15,
    BTFE_FMT_ASTC_4X4       = 16,
    BTFE_FMT_BC1            = 17,
    BTFE_FMT_BC3            = 18,
    BTFE_FMT_BC5            = 19,
    BTFE_FMT_ETC2           = 20
} btfe_format_t;

/* Channel Swizzle Modes (Phase 5D Spec) */
typedef enum {
    BTFE_SWIZZLE_RGBA       = 0,
    BTFE_SWIZZLE_BGRA       = 1,
    BTFE_SWIZZLE_ARGB       = 2,
    BTFE_SWIZZLE_ABGR       = 3,
    BTFE_SWIZZLE_CUSTOM     = 4,
    BTFE_SWIZZLE_IDENTITY   = 5
} btfe_swizzle_mode_t;

/* Swizzle Channel Mapping Vector */
typedef struct {
    uint8_t r_src; /* 0=R, 1=G, 2=B, 3=A, 4=0, 5=1 */
    uint8_t g_src;
    uint8_t b_src;
    uint8_t a_src;
} btfe_swizzle_matrix_t;

/* Tile Layout Modes (Phase 5F Spec) */
typedef enum {
    BTFE_TILE_LINEAR        = 0,
    BTFE_TILE_X_TILE        = 1,
    BTFE_TILE_Y_TILE        = 2,
    BTFE_TILE_MORTON        = 3,
    BTFE_TILE_BLOCK_LINEAR  = 4,
    BTFE_TILE_VENDOR_NATIVE = 5
} btfe_tile_mode_t;

/* Mip Level Descriptor */
typedef struct {
    uint32_t level;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    size_t   size_bytes;
    size_t   offset_bytes;
    uint32_t row_pitch_bytes;
} btfe_mip_desc_t;

/* Mipchain Descriptor (Phase 5E Spec) */
typedef struct {
    uint32_t        mip_count;
    size_t          total_memory_bytes;
    btfe_mip_desc_t mips[BTFE_MAX_MIP_LEVELS];
} btfe_mipchain_t;

/* Texture Sampler Filtering & Address Modes (Phase 5I Spec) */
typedef enum { BTFE_FILTER_NEAREST = 0, BTFE_FILTER_LINEAR = 1, BTFE_FILTER_BILINEAR = 2, BTFE_FILTER_TRILINEAR = 3, BTFE_FILTER_ANISOTROPIC = 4 } btfe_filter_mode_t;
typedef enum { BTFE_WRAP_REPEAT = 0, BTFE_WRAP_CLAMP = 1, BTFE_WRAP_MIRROR = 2, BTFE_WRAP_BORDER = 3 } btfe_wrap_mode_t;

/* Sampler State Descriptor */
typedef struct {
    btfe_filter_mode_t min_filter;
    btfe_filter_mode_t mag_filter;
    btfe_wrap_mode_t   wrap_u;
    btfe_wrap_mode_t   wrap_v;
    btfe_wrap_mode_t   wrap_w;
    float              max_anisotropy;
    float              lod_bias;
    uint32_t           border_color_rgba;
} btfe_sampler_desc_t;

/* Texture View Types (Phase 5H Spec) */
typedef enum {
    BTFE_VIEW_BASE_LEVEL    = 0,
    BTFE_VIEW_MIP_LEVEL     = 1,
    BTFE_VIEW_ARRAY_SLICE   = 2,
    BTFE_VIEW_DEPTH_SLICE   = 3,
    BTFE_VIEW_READ_ONLY     = 4,
    BTFE_VIEW_RENDER_TARGET = 5,
    BTFE_VIEW_SHADER_RESOURCE = 6
} btfe_view_type_t;

/* Texture Creation Info */
typedef struct {
    uint32_t            width;
    uint32_t            height;
    uint32_t            depth;
    uint32_t            array_layers;
    uint32_t            mip_levels;
    btfe_format_t       format;
    btfe_swizzle_mode_t swizzle;
    btfe_tile_mode_t    tile_mode;
    uint32_t            usage_flags;
    bvmm_domain_t       preferred_domain;
    uint32_t            owner_pid;
} btfe_texture_create_info_t;

/* Primary Texture Descriptor Structure */
typedef struct btfe_texture_desc {
    uint32_t            canary_magic;    /* 0x54455854 */
    btfe_texture_id_t   texture_id;
    uint16_t            generation_id;
    uint32_t            registry_index;
    uint32_t            width;
    uint32_t            height;
    uint32_t            depth;
    uint32_t            array_layers;
    btfe_format_t       format;
    btfe_swizzle_mode_t swizzle_mode;
    btfe_swizzle_matrix_t swizzle_matrix;
    btfe_tile_mode_t    tile_mode;
    btfe_mipchain_t     mipchain;
    bvmm_surface_id_t   surface_id;      /* Exactly one BSME Surface */
    btfe_sampler_id_t   sampler_id;
    uint32_t            ref_count;
    uint32_t            owner_pid;
    uint64_t            creation_time;
    uint64_t            last_access_time;
} btfe_texture_desc_t;

/* Texture View Descriptor */
typedef struct {
    btfe_texture_id_t   base_texture_id;
    btfe_view_type_t    view_type;
    uint32_t            base_mip_level;
    uint32_t            mip_level_count;
    uint32_t            base_array_layer;
    uint32_t            array_layer_count;
    btfe_format_t       view_format;
} btfe_texture_view_desc_t;

/* BTFE Diagnostics Summary */
typedef struct {
    uint32_t            total_textures_created;
    uint32_t            alive_textures;
    uint32_t            destroyed_textures;
    uint32_t            peak_textures;
    size_t              total_texture_memory_bytes;
    size_t              mip_memory_bytes;
    uint32_t            format_histogram[21];
    uint32_t            tile_histogram[6];
    uint64_t            upload_bytes_total;
    uint32_t            upload_count;
    uint32_t            failed_uploads;
    uint32_t            validation_failures;
} btfe_diagnostics_t;

/**
 * @brief Initialize the Production Texture & Format Engine (BTFE).
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_init(void);

/**
 * @brief Shutdown BTFE and clean up all registered texture objects.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_shutdown(void);

/**
 * @brief Create a GPU texture object backed by an active BSME Surface.
 * @param info Creation parameters.
 * @param out_texture_id Pointer to receive unique 64-bit Texture ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_create(const btfe_texture_create_info_t* info, btfe_texture_id_t* out_texture_id);

/**
 * @brief Destroy a texture object and release its backing BSME Surface.
 * @param texture_id Target Texture ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_destroy(btfe_texture_id_t texture_id);

/**
 * @brief Fast O(1) lookup of texture descriptor by Texture ID.
 * @param texture_id Target Texture ID.
 * @param out_desc Pointer to receive descriptor pointer.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_lookup(btfe_texture_id_t texture_id, btfe_texture_desc_t** out_desc);

/**
 * @brief Increment texture reference count.
 * @param texture_id Target Texture ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_ref_inc(btfe_texture_id_t texture_id);

/**
 * @brief Decrement texture reference count. Auto-destroys when refcount hits 0.
 * @param texture_id Target Texture ID.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_ref_dec(btfe_texture_id_t texture_id);

/**
 * @brief Upload pixel data into a texture subresource.
 * @param texture_id Target Texture ID.
 * @param mip_level Target mip level.
 * @param src_pixels Pointer to CPU source buffer.
 * @param src_size_bytes Size of source buffer in bytes.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_upload(btfe_texture_id_t texture_id, uint32_t mip_level, const void* src_pixels, size_t src_size_bytes);

/**
 * @brief Create a specialized texture view descriptor.
 * @param texture_id Base Texture ID.
 * @param view_type Desired view type.
 * @param out_view Pointer to receive view descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_texture_create_view(btfe_texture_id_t texture_id, btfe_view_type_t view_type, btfe_texture_view_desc_t* out_view);

/**
 * @brief Compute mipchain layout offsets for given dimensions and format.
 * @param width Base width.
 * @param height Base height.
 * @param requested_mips Mip count.
 * @param format Texture format.
 * @param out_mipchain Pointer to receive calculated mipchain descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_compute_mipchain(uint32_t width, uint32_t height, uint32_t requested_mips, btfe_format_t format, btfe_mipchain_t* out_mipchain);

/**
 * @brief Dump detailed developer inspection log for a specific texture.
 * @param texture_id Target Texture ID.
 */
void btfe_texture_dump(btfe_texture_id_t texture_id);

/**
 * @brief Dump all registered active textures in the system.
 */
void btfe_texture_dump_all(void);

/**
 * @brief Retrieve current BTFE diagnostics summary.
 * @param out_diag Pointer to receive diagnostics descriptor.
 * @return BVMM_SUCCESS on success, error code otherwise.
 */
bvmm_result_t btfe_get_diagnostics(btfe_diagnostics_t* out_diag);

/**
 * @brief Validate integrity of all registered textures.
 * @return true if valid, false if corruption detected.
 */
bool btfe_texture_validate_all(void);

#ifdef __cplusplus
}
#endif

#endif /* _BOS_BVMM_TEXTURE_H_ */
