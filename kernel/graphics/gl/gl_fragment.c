#include "kernel/graphics/gl/gl_fragment.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_depth.h"
#include "kernel/graphics/gl/gl_sampler.h"
#include "kernel/graphics/gl/gl_math.h"

static float gl_expf(float x) {
    if (x >= 0.0f) return 1.0f;
    if (x <= -10.0f) return 0.0f;

    // (1 + x/32)^32 approximation via 5 squarings
    float val = 1.0f + x * (1.0f / 32.0f);
    if (val <= 0.0f) return 0.0f;

    val *= val; // 2
    val *= val; // 4
    val *= val; // 8
    val *= val; // 16
    val *= val; // 32
    return val;
}

static void gl_get_blend_factor(GLenum factor, const float src[4], const float dst[4], float out_f[4]) {
    switch (factor) {
        case GL_ZERO:
            out_f[0] = 0.0f; out_f[1] = 0.0f; out_f[2] = 0.0f; out_f[3] = 0.0f;
            break;
        case GL_ONE:
            out_f[0] = 1.0f; out_f[1] = 1.0f; out_f[2] = 1.0f; out_f[3] = 1.0f;
            break;
        case GL_SRC_COLOR:
            out_f[0] = src[0]; out_f[1] = src[1]; out_f[2] = src[2]; out_f[3] = src[3];
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            out_f[0] = 1.0f - src[0]; out_f[1] = 1.0f - src[1]; out_f[2] = 1.0f - src[2]; out_f[3] = 1.0f - src[3];
            break;
        case GL_DST_COLOR:
            out_f[0] = dst[0]; out_f[1] = dst[1]; out_f[2] = dst[2]; out_f[3] = dst[3];
            break;
        case GL_ONE_MINUS_DST_COLOR:
            out_f[0] = 1.0f - dst[0]; out_f[1] = 1.0f - dst[1]; out_f[2] = 1.0f - dst[2]; out_f[3] = 1.0f - dst[3];
            break;
        case GL_SRC_ALPHA:
            out_f[0] = src[3]; out_f[1] = src[3]; out_f[2] = src[3]; out_f[3] = src[3];
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            out_f[0] = 1.0f - src[3]; out_f[1] = 1.0f - src[3]; out_f[2] = 1.0f - src[3]; out_f[3] = 1.0f - src[3];
            break;
        case GL_DST_ALPHA:
            out_f[0] = dst[3]; out_f[1] = dst[3]; out_f[2] = dst[3]; out_f[3] = dst[3];
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            out_f[0] = 1.0f - dst[3]; out_f[1] = 1.0f - dst[3]; out_f[2] = 1.0f - dst[3]; out_f[3] = 1.0f - dst[3];
            break;
        case GL_SRC_ALPHA_SATURATE: {
            float sat = src[3] < (1.0f - dst[3]) ? src[3] : (1.0f - dst[3]);
            out_f[0] = sat; out_f[1] = sat; out_f[2] = sat; out_f[3] = 1.0f;
            break;
        }
        default:
            out_f[0] = 1.0f; out_f[1] = 1.0f; out_f[2] = 1.0f; out_f[3] = 1.0f;
            break;
    }
}

bool gl_fragment_process(GLContextState* state, const GLRenderTarget* target, const GLFragment* frag) {
    if (!state || !target || !frag) return false;

    state->fragments_generated++;

    int x = frag->x;
    int y = frag->y;

    // 1. Target Bounds Check
    if (x < 0 || x >= (int)target->width || y < 0 || y >= (int)target->height) {
        return false;
    }

    // 2. Scissor Test (Converting OpenGL bottom-left origin to Top-Left screen coords)
    if (state->scissor_test_enabled) {
        int top_y    = (int)target->height - (state->scissor_y + state->scissor_h);
        int bottom_y = (int)target->height - state->scissor_y;

        if (x < state->scissor_x || x >= state->scissor_x + state->scissor_w ||
            y < top_y || y >= bottom_y) {
            state->fragments_scissor_rejected++;
            return false;
        }
    }

    float r = frag->color[0];
    float g = frag->color[1];
    float b = frag->color[2];
    float a = frag->color[3];

    // 3. Texture Sampling & Environment
    if (state->texture_2d_enabled && frag->has_texture) {
        GLTextureObject* active_tex = gl_state_get_bound_texture(state);
        if (active_tex && active_tex->defined && active_tex->levels[0].defined && active_tex->levels[0].pixel_data) {
            // Feedback Loop Hazard Detection
            if (!gl_fbo_is_sampler_aliased(state, active_tex)) {
                float tex_rgba[4];
                gl_sample_texture_lod(active_tex, frag->texcoord[0], frag->texcoord[1], frag->lod, tex_rgba);
                state->texture_samples++;
                if (active_tex->min_filter == GL_NEAREST) state->nearest_samples++;
                else state->linear_samples++;

                if (state->texture_env_mode == GL_REPLACE) {
                    r = tex_rgba[0]; g = tex_rgba[1]; b = tex_rgba[2]; a = tex_rgba[3];
                } else { // GL_MODULATE
                    r *= tex_rgba[0]; g *= tex_rgba[1]; b *= tex_rgba[2]; a *= tex_rgba[3];
                }
            }
        }
    }

    // 4. Fixed-Function Fog Application
    if (state->fog_enabled) {
        float z_fog = frag->fog_coord;
        float f = 1.0f;

        if (state->fog_mode == GL_LINEAR) {
            float diff = state->fog_end - state->fog_start;
            if (gl_fabsf(diff) > 1e-7f) {
                f = (state->fog_end - z_fog) / diff;
            }
        } else if (state->fog_mode == GL_EXP) {
            f = gl_expf(-state->fog_density * z_fog);
        } else if (state->fog_mode == GL_EXP2) {
            float dz = state->fog_density * z_fog;
            f = gl_expf(-(dz * dz));
        }

        if (f < 0.0f) f = 0.0f;
        if (f > 1.0f) f = 1.0f;

        r = f * r + (1.0f - f) * state->fog_color[0];
        g = f * g + (1.0f - f) * state->fog_color[1];
        b = f * b + (1.0f - f) * state->fog_color[2];
        // Alpha is preserved
    }

    // 5. Alpha Test
    if (state->alpha_test_enabled) {
        float ref = state->alpha_ref;
        bool alpha_pass = false;

        switch (state->alpha_func) {
            case GL_NEVER:    alpha_pass = false; break;
            case GL_LESS:     alpha_pass = (a < ref); break;
            case GL_EQUAL:    alpha_pass = (gl_fabsf(a - ref) <= 1e-5f); break;
            case GL_LEQUAL:   alpha_pass = (a <= ref + 1e-5f); break;
            case GL_GREATER:  alpha_pass = (a > ref); break;
            case GL_NOTEQUAL: alpha_pass = (gl_fabsf(a - ref) > 1e-5f); break;
            case GL_GEQUAL:   alpha_pass = (a >= ref - 1e-5f); break;
            case GL_ALWAYS:   alpha_pass = true; break;
            default:          alpha_pass = true; break;
        }

        if (!alpha_pass) {
            state->fragments_alpha_rejected++;
            return false; // Rejected fragments MUST NOT update depth/stencil or participate in blending
        }
    }

    uint8_t*  stencil_ptr = (target->has_stencil && target->stencil_buffer) ? &target->stencil_buffer[(size_t)y * (size_t)target->stencil_pitch + (size_t)x] : NULL;
    float*    depth_ptr   = (target->has_depth && target->depth_buffer) ? &target->depth_buffer[(size_t)y * (size_t)target->depth_pitch + (size_t)x] : NULL;
    uint32_t* color_ptr   = (target->has_color && target->color_buffer) ? &target->color_buffer[(size_t)y * (size_t)target->color_pitch + (size_t)x] : NULL;

    // Helper for applying stencil operation
    #define APPLY_STENCIL_OP(op) do { \
        if (state->stencil_test_enabled && stencil_ptr) { \
            uint8_t old_s = *stencil_ptr; \
            uint8_t new_s = old_s; \
            uint8_t ref_s = (uint8_t)(state->stencil_ref & 0xFF); \
            switch (op) { \
                case GL_KEEP: break; \
                case GL_ZERO: new_s = 0; break; \
                case GL_REPLACE: new_s = ref_s; break; \
                case GL_INCR: new_s = (old_s < 255) ? (old_s + 1) : 255; break; \
                case GL_DECR: new_s = (old_s > 0) ? (old_s - 1) : 0; break; \
                case GL_INVERT: new_s = (uint8_t)(~old_s); break; \
                case GL_INCR_WRAP: new_s = (uint8_t)((old_s + 1) & 0xFF); break; \
                case GL_DECR_WRAP: new_s = (uint8_t)((old_s - 1) & 0xFF); break; \
            } \
            uint8_t wmask = (uint8_t)(state->stencil_write_mask & 0xFF); \
            *stencil_ptr = (old_s & ~wmask) | (new_s & wmask); \
            state->stencil_updates++; \
        } \
    } while (0)

    // 6. Stencil Test
    bool stencil_pass = true;
    if (state->stencil_test_enabled && stencil_ptr) {
        uint8_t s_val = *stencil_ptr;
        uint8_t m_ref = (uint8_t)(state->stencil_ref & state->stencil_value_mask & 0xFF);
        uint8_t m_val = (uint8_t)(s_val & state->stencil_value_mask & 0xFF);

        switch (state->stencil_func) {
            case GL_NEVER:    stencil_pass = false; break;
            case GL_LESS:     stencil_pass = (m_ref < m_val); break;
            case GL_EQUAL:    stencil_pass = (m_ref == m_val); break;
            case GL_LEQUAL:   stencil_pass = (m_ref <= m_val); break;
            case GL_GREATER:  stencil_pass = (m_ref > m_val); break;
            case GL_NOTEQUAL: stencil_pass = (m_ref != m_val); break;
            case GL_GEQUAL:   stencil_pass = (m_ref >= m_val); break;
            case GL_ALWAYS:   stencil_pass = true; break;
            default:          stencil_pass = true; break;
        }
    }

    if (!stencil_pass) {
        state->fragments_stencil_rejected++;
        APPLY_STENCIL_OP(state->stencil_fail_op);
        return false; // Stencil test fail: NO depth test, NO color write, NO depth write
    }

    // 7. Depth Test
    bool depth_pass = true;
    if (state->depth_test_enabled && depth_ptr) {
        float buf_z = *depth_ptr;
        switch (state->depth_func) {
            case GL_NEVER:    depth_pass = false; break;
            case GL_LESS:     depth_pass = (frag->depth < buf_z); break;
            case GL_EQUAL:    depth_pass = (frag->depth == buf_z); break;
            case GL_LEQUAL:   depth_pass = (frag->depth <= buf_z); break;
            case GL_GREATER:  depth_pass = (frag->depth > buf_z); break;
            case GL_NOTEQUAL: depth_pass = (frag->depth != buf_z); break;
            case GL_GEQUAL:   depth_pass = (frag->depth >= buf_z); break;
            case GL_ALWAYS:   depth_pass = true; break;
            default:          depth_pass = (frag->depth < buf_z); break;
        }
    }

    if (!depth_pass) {
        state->fragments_depth_rejected++;
        APPLY_STENCIL_OP(state->stencil_depth_fail_op);
        return false; // Depth test fail: NO color write, NO depth write
    }

    // Both Stencil and Depth passed! Apply dppass operation
    APPLY_STENCIL_OP(state->stencil_depth_pass_op);

    // Depth Write (only if depth testing enabled AND writemask enabled)
    if (state->depth_test_enabled && state->depth_writemask && depth_ptr) {
        *depth_ptr = frag->depth;
        state->depth_writes++;
    }

    if (!color_ptr) {
        state->rasterizer_pixels_written++;
        state->fragments_written++;
        return true;
    }

    // 8. Blending
    if (state->blend_enabled) {
        uint32_t dst_argb = *color_ptr;
        float dst_rgba[4] = {
            (float)((dst_argb >> 16) & 0xFF) / 255.0f,
            (float)((dst_argb >> 8)  & 0xFF) / 255.0f,
            (float)(dst_argb         & 0xFF) / 255.0f,
            (float)((dst_argb >> 24) & 0xFF) / 255.0f
        };

        float src_rgba[4] = {r, g, b, a};
        float f_src[4], f_dst[4];

        gl_get_blend_factor(state->blend_src_factor, src_rgba, dst_rgba, f_src);
        gl_get_blend_factor(state->blend_dst_factor, src_rgba, dst_rgba, f_dst);

        r = src_rgba[0] * f_src[0] + dst_rgba[0] * f_dst[0];
        g = src_rgba[1] * f_src[1] + dst_rgba[1] * f_dst[1];
        b = src_rgba[2] * f_src[2] + dst_rgba[2] * f_dst[2];
        a = src_rgba[3] * f_src[3] + dst_rgba[3] * f_dst[3];

        state->fragments_blended++;
    }

    // 9. Color Write Mask
    if (color_ptr && (!state->color_mask[0] || !state->color_mask[1] || !state->color_mask[2] || !state->color_mask[3])) {
        uint32_t dst_argb = *color_ptr;
        float dst_r = (float)((dst_argb >> 16) & 0xFF) / 255.0f;
        float dst_g = (float)((dst_argb >> 8)  & 0xFF) / 255.0f;
        float dst_b = (float)(dst_argb         & 0xFF) / 255.0f;
        float dst_a = (float)((dst_argb >> 24) & 0xFF) / 255.0f;

        if (!state->color_mask[0]) r = dst_r;
        if (!state->color_mask[1]) g = dst_g;
        if (!state->color_mask[2]) b = dst_b;
        if (!state->color_mask[3]) a = dst_a;
    }

    // Clamp final RGBA to [0, 1]
    if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f; if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f; if (b > 1.0f) b = 1.0f;
    if (a < 0.0f) a = 0.0f; if (a > 1.0f) a = 1.0f;

    uint32_t ur = (uint32_t)(r * 255.0f);
    uint32_t ug = (uint32_t)(g * 255.0f);
    uint32_t ub = (uint32_t)(b * 255.0f);
    uint32_t ua = (uint32_t)(a * 255.0f);

    // 10. Final Color Write
    if (color_ptr) {
        *color_ptr = (ua << 24) | (ur << 16) | (ug << 8) | ub;
    }

    state->rasterizer_pixels_written++;
    state->fragments_written++;
    return true;
}
