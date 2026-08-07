#include "bvmm_texture.h"

uint32_t btfe_format_bpp(btfe_format_t fmt) {
    switch (fmt) {
        case BTFE_FMT_R8:
        case BTFE_FMT_STENCIL8:       return 1;
        case BTFE_FMT_RGB565:
        case BTFE_FMT_RG8:
        case BTFE_FMT_DEPTH16:        return 2;
        case BTFE_FMT_RGB8:           return 3;
        case BTFE_FMT_RGBA8888:
        case BTFE_FMT_BGRA8888:
        case BTFE_FMT_ARGB8888:
        case BTFE_FMT_ABGR8888:
        case BTFE_FMT_RGBA16:
        case BTFE_FMT_RGBA16F:
        case BTFE_FMT_DEPTH24:
        case BTFE_FMT_DEPTH32:
        case BTFE_FMT_DEPTH24STENCIL8: return 4;
        case BTFE_FMT_RGBA32F:        return 16;
        case BTFE_FMT_BC1:            return 1; /* 8 bytes per 4x4 block */
        case BTFE_FMT_BC3:
        case BTFE_FMT_BC5:
        case BTFE_FMT_ASTC_4X4:
        case BTFE_FMT_ETC2:           return 1; /* 16 bytes per 4x4 block */
        default:                      return 4;
    }
}

btfe_swizzle_matrix_t btfe_get_swizzle_matrix(btfe_swizzle_mode_t mode) {
    btfe_swizzle_matrix_t m;
    switch (mode) {
        case BTFE_SWIZZLE_BGRA:
            m.r_src = 2; m.g_src = 1; m.b_src = 0; m.a_src = 3; break;
        case BTFE_SWIZZLE_ARGB:
            m.r_src = 1; m.g_src = 2; m.b_src = 3; m.a_src = 0; break;
        case BTFE_SWIZZLE_ABGR:
            m.r_src = 3; m.g_src = 2; m.b_src = 1; m.a_src = 0; break;
        case BTFE_SWIZZLE_RGBA:
        case BTFE_SWIZZLE_IDENTITY:
        default:
            m.r_src = 0; m.g_src = 1; m.b_src = 2; m.a_src = 3; break;
    }
    return m;
}

bvmm_result_t btfe_compute_mipchain(uint32_t width, uint32_t height, uint32_t requested_mips, btfe_format_t format, btfe_mipchain_t* out_mipchain) {
    if (width == 0 || height == 0 || !out_mipchain) return BVMM_ERR_INVALID_ARGUMENT;

    /* Calculate maximum physical mip count */
    uint32_t max_dim = (width > height) ? width : height;
    uint32_t max_possible_mips = 1;
    while ((max_dim >> (max_possible_mips - 1)) > 1) {
        max_possible_mips++;
    }

    uint32_t actual_mips = (requested_mips == 0 || requested_mips > max_possible_mips) ? max_possible_mips : requested_mips;
    if (actual_mips > BTFE_MAX_MIP_LEVELS) actual_mips = BTFE_MAX_MIP_LEVELS;

    out_mipchain->mip_count = actual_mips;
    size_t current_offset = 0;
    uint32_t bpp = btfe_format_bpp(format);

    for (uint32_t i = 0; i < actual_mips; i++) {
        uint32_t mip_w = (width >> i) > 0 ? (width >> i) : 1;
        uint32_t mip_h = (height >> i) > 0 ? (height >> i) : 1;

        uint32_t row_pitch = (mip_w * bpp + 15) & ~15U; /* 16-byte aligned row pitch */
        size_t level_size = (size_t)row_pitch * mip_h;

        /* Align each mip level to 256 bytes for GPU HW requirement */
        size_t aligned_offset = (current_offset + 255) & ~255ULL;

        out_mipchain->mips[i].level = i;
        out_mipchain->mips[i].width = mip_w;
        out_mipchain->mips[i].height = mip_h;
        out_mipchain->mips[i].depth = 1;
        out_mipchain->mips[i].row_pitch_bytes = row_pitch;
        out_mipchain->mips[i].size_bytes = level_size;
        out_mipchain->mips[i].offset_bytes = aligned_offset;

        current_offset = aligned_offset + level_size;
    }

    out_mipchain->total_memory_bytes = current_offset;
    return BVMM_SUCCESS;
}

/* Morton Z-Order coordinate mapping (Phase 5F Spec) */
uint32_t btfe_morton_z_order(uint32_t x, uint32_t y) {
    x = (x | (x << 8)) & 0x00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;

    y = (y | (y << 8)) & 0x00FF00FF;
    y = (y | (y << 4)) & 0x0F0F0F0F;
    y = (y | (y << 2)) & 0x33333333;
    y = (y | (y << 1)) & 0x55555555;

    return x | (y << 1);
}
