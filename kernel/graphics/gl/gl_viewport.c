#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/gl/gl_math.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/bgl/bgl.h"

bool gl_viewport_transform_vertex(const GLVertex* clip_v, int vp_x, int vp_y, int vp_w, int vp_h, GLScreenVertex* out_v) {
    if (!clip_v || !out_v) return false;

    int eff_w = vp_w;
    int eff_h = vp_h;
    if (eff_w <= 0 || eff_h <= 0) {
        BGLContext* bgl_ctx = bglGetCurrentContext();
        if (bgl_ctx && bgl_ctx->bound_drawable) {
            if (eff_w <= 0) eff_w = (int)bgl_ctx->bound_drawable->width;
            if (eff_h <= 0) eff_h = (int)bgl_ctx->bound_drawable->height;
        }
    }
    if (eff_w <= 0 || eff_h <= 0) return false;

    float w = clip_v->clip_pos.w;
    if (gl_fabsf(w) < 1e-7f) return false;

    float inv_w = 1.0f / w;

    // 1. Perspective Divide -> Normalized Device Coordinates (NDC)
    float ndc_x = clip_v->clip_pos.x * inv_w;
    float ndc_y = clip_v->clip_pos.y * inv_w;
    float ndc_z = clip_v->clip_pos.z * inv_w;

    // Guard against NaN or Inf
    if (ndc_x != ndc_x || ndc_y != ndc_y || ndc_z != ndc_z) return false;

    // 2. Viewport Mapping -> Window Coordinates (Top-Left BGL memory origin)
    out_v->x = (float)vp_x + (ndc_x + 1.0f) * 0.5f * (float)eff_w;
    out_v->y = (float)vp_y + (1.0f - ndc_y) * 0.5f * (float)eff_h;

    // Depth Range Mapping (nearVal to farVal)
    GLContextState* state = gl_state_get_current();
    float n_val = state ? (float)state->depth_range_near : 0.0f;
    float f_val = state ? (float)state->depth_range_far  : 1.0f;

    out_v->z = n_val + (ndc_z + 1.0f) * 0.5f * (f_val - n_val);

    // Clamp depth z to [0, 1]
    if (out_v->z < 0.0f) out_v->z = 0.0f;
    if (out_v->z > 1.0f) out_v->z = 1.0f;

    out_v->inv_w        = inv_w;
    out_v->s_over_w     = clip_v->texcoord[0] * inv_w;
    out_v->t_over_w     = clip_v->texcoord[1] * inv_w;
    out_v->fog_z_over_w = clip_v->fog_coord * inv_w;

    out_v->r = clip_v->color[0];
    out_v->g = clip_v->color[1];
    out_v->b = clip_v->color[2];
    out_v->a = clip_v->color[3];

    return true;
}
