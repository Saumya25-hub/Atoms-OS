#include "../include/bvmm.h"
#include "../texture/bvmm_texture.h"
#include "kernel/drivers/display/display.h"

/**
 * @file bvmm_phase5_tests.c
 * @brief Production Certification Tests for BVMM Phase 5 (Production Texture & Format Engine BTFE V1.0)
 */

bool bvmm_run_phase5_tests(void) {
    display_print("[BVMM TEST] Starting Phase 5 Production Texture Engine (BTFE) Certification Suite...\n");

    if (!bvmm_is_initialized()) {
        bvmm_init();
    }

    /* 1. Texture Creation & BSME Surface Backing Test */
    btfe_texture_create_info_t create_info = {
        .width            = 2048,
        .height           = 2048,
        .mip_levels       = 0, /* Auto calculate all mips */
        .format           = BTFE_FMT_RGBA8888,
        .swizzle          = BTFE_SWIZZLE_RGBA,
        .tile_mode        = BTFE_TILE_MORTON,
        .preferred_domain = BVMM_DOMAIN_VRAM,
        .owner_pid        = 1
    };

    btfe_texture_id_t tex_id = BTFE_INVALID_TEXTURE_ID;
    bvmm_result_t res = btfe_texture_create(&create_info, &tex_id);
    if (res != BVMM_SUCCESS || tex_id == BTFE_INVALID_TEXTURE_ID) {
        display_print("[BVMM TEST] FAIL: Texture creation failed\n");
        return false;
    }

    /* 2. Lookup & Descriptor Verification */
    btfe_texture_desc_t* desc = NULL;
    res = btfe_texture_lookup(tex_id, &desc);
    if (res != BVMM_SUCCESS || !desc || desc->width != 2048 || desc->surface_id == BVMM_INVALID_SURFACE_ID) {
        display_print("[BVMM TEST] FAIL: Texture descriptor or BSME surface backing lookup failed\n");
        return false;
    }

    /* 3. Mipchain Verification (2048x2048 = 12 mip levels) */
    if (desc->mipchain.mip_count != 12) {
        display_print("[BVMM TEST] FAIL: Mipchain calculation mismatch (expected 12 levels)\n");
        return false;
    }

    /* 4. Large 8192x8192 Texture Mipchain Test */
    btfe_mipchain_t mipchain_8k;
    res = btfe_compute_mipchain(8192, 8192, 0, BTFE_FMT_RGBA8888, &mipchain_8k);
    if (res != BVMM_SUCCESS || mipchain_8k.mip_count != 14) {
        display_print("[BVMM TEST] FAIL: 8K texture mipchain calculation failed\n");
        return false;
    }

    /* 5. Subresource Upload Engine Test */
    uint8_t sample_pixels[256];
    for (int i = 0; i < 256; i++) sample_pixels[i] = (uint8_t)(i & 0xFF);

    res = btfe_texture_upload(tex_id, 0, sample_pixels, sizeof(sample_pixels));
    if (res != BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Texture subresource upload failed\n");
        return false;
    }

    /* 6. Texture View Creation Test */
    btfe_texture_view_desc_t view_desc;
    res = btfe_texture_create_view(tex_id, BTFE_VIEW_SHADER_RESOURCE, &view_desc);
    if (res != BVMM_SUCCESS || view_desc.base_texture_id != tex_id) {
        display_print("[BVMM TEST] FAIL: Texture view creation failed\n");
        return false;
    }

    /* 7. Reference Counting & Auto Zero-Ref Cleanup Test */
    btfe_texture_ref_inc(tex_id);
    if (desc->ref_count != 2) {
        display_print("[BVMM TEST] FAIL: Texture refcount increment failed\n");
        return false;
    }
    btfe_texture_ref_dec(tex_id);
    btfe_texture_ref_dec(tex_id); /* RefCount becomes 0 -> Auto-destroyed */

    /* Lookup should now fail due to zero-ref destruction */
    btfe_texture_desc_t* dead_desc = NULL;
    if (btfe_texture_lookup(tex_id, &dead_desc) == BVMM_SUCCESS) {
        display_print("[BVMM TEST] FAIL: Texture auto zero-ref destruction failed\n");
        return false;
    }

    /* 8. 10,000 Texture Creation & Destruction Stress Loop */
    display_print("[BVMM TEST] Executing 10,000 Texture Creation & Destruction Stress Test...\n");
    btfe_texture_create_info_t stress_info = {
        .width            = 64,
        .height           = 64,
        .mip_levels       = 1,
        .format           = BTFE_FMT_RGBA8888,
        .swizzle          = BTFE_SWIZZLE_RGBA,
        .tile_mode        = BTFE_TILE_LINEAR,
        .owner_pid        = 1
    };

    for (int i = 0; i < 10000; i++) {
        btfe_texture_id_t stress_id = BTFE_INVALID_TEXTURE_ID;
        res = btfe_texture_create(&stress_info, &stress_id);
        if (res != BVMM_SUCCESS || stress_id == BTFE_INVALID_TEXTURE_ID) {
            display_print("[BVMM TEST] FAIL: 10,000 texture stress test failed at iteration ");
            display_print_dec((uint32_t)i);
            display_print("\n");
            return false;
        }
        btfe_texture_destroy(stress_id);
    }
    display_print("[BVMM TEST] 10,000 Texture Stress Test: PASS\n");

    /* 9. Diagnostics & Inspector Dump */
    btfe_texture_dump_all();

    display_print("[BVMM TEST] PASS: All Phase 5 Production Texture Engine Certification Tests Passed!\n");
    return true;
}
