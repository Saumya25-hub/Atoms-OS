#include "kernel/graphics/gl/gl_sampler.h"
#include "kernel/graphics/gl/gl_math.h"

static float apply_wrap_mode(float coord, GLenum wrap_mode) {
    if (wrap_mode == GL_REPEAT) {
        float f = coord - (float)((int)coord);
        if (f < 0.0f) f += 1.0f;
        return f;
    } else { // GL_CLAMP / GL_CLAMP_TO_EDGE
        if (coord < 0.0f) return 0.0f;
        if (coord > 1.0f) return 1.0f;
        return coord;
    }
}

static inline void get_texel_rgba_level(const GLTextureLevel* lvl, int x, int y, float out[4]) {
    if (!lvl || !lvl->pixel_data || lvl->width <= 0 || lvl->height <= 0) {
        out[0] = 1.0f; out[1] = 1.0f; out[2] = 1.0f; out[3] = 1.0f;
        return;
    }

    if (x < 0) x = 0; if (x >= (int)lvl->width)  x = (int)lvl->width - 1;
    if (y < 0) y = 0; if (y >= (int)lvl->height) y = (int)lvl->height - 1;

    size_t idx = ((size_t)y * (size_t)lvl->width + (size_t)x) * 4;
    out[0] = (float)lvl->pixel_data[idx + 2] / 255.0f; // Red
    out[1] = (float)lvl->pixel_data[idx + 1] / 255.0f; // Green
    out[2] = (float)lvl->pixel_data[idx + 0] / 255.0f; // Blue
    out[3] = (float)lvl->pixel_data[idx + 3] / 255.0f; // Alpha
}

static void sample_level_nearest(const GLTextureLevel* lvl, float su, float sv, float out_rgba[4]) {
    int ix = (int)(su * (float)lvl->width);
    int iy = (int)(sv * (float)lvl->height);
    get_texel_rgba_level(lvl, ix, iy, out_rgba);
}

static void sample_level_bilinear(const GLTextureLevel* lvl, float su, float sv, float out_rgba[4]) {
    float fx = su * (float)lvl->width - 0.5f;
    float fy = sv * (float)lvl->height - 0.5f;

    int x0 = (int)fx; if (fx < 0.0f && fx != (float)x0) x0--;
    int y0 = (int)fy; if (fy < 0.0f && fy != (float)y0) y0--;
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float u_frac = fx - (float)x0;
    float v_frac = fy - (float)y0;

    float c00[4], c10[4], c01[4], c11[4];
    get_texel_rgba_level(lvl, x0, y0, c00);
    get_texel_rgba_level(lvl, x1, y0, c10);
    get_texel_rgba_level(lvl, x0, y1, c01);
    get_texel_rgba_level(lvl, x1, y1, c11);

    float w00 = (1.0f - u_frac) * (1.0f - v_frac);
    float w10 = u_frac * (1.0f - v_frac);
    float w01 = (1.0f - u_frac) * v_frac;
    float w11 = u_frac * v_frac;

    for (int i = 0; i < 4; i++) {
        out_rgba[i] = w00 * c00[i] + w10 * c10[i] + w01 * c01[i] + w11 * c11[i];
    }
}

static int get_max_valid_mip_level(const GLTextureObject* tex) {
    int max_lvl = 0;
    for (int i = 0; i < GL_MAX_MIP_LEVELS; i++) {
        if (tex->levels[i].defined && tex->levels[i].pixel_data) {
            max_lvl = i;
        } else {
            break;
        }
    }
    return max_lvl;
}

void gl_sample_texture_lod(const GLTextureObject* tex, float u, float v, float lod, float out_rgba[4]) {
    if (!tex || !tex->defined || !out_rgba || !gl_texture_is_complete(tex)) {
        if (out_rgba) {
            out_rgba[0] = 1.0f; out_rgba[1] = 1.0f; out_rgba[2] = 1.0f; out_rgba[3] = 1.0f;
        }
        return;
    }

    float su = apply_wrap_mode(u, tex->wrap_s);
    float sv = apply_wrap_mode(v, tex->wrap_t);
    int max_lvl = get_max_valid_mip_level(tex);

    if (lod <= 0.0f) {
        // Magnification
        if (tex->mag_filter == GL_NEAREST) {
            sample_level_nearest(&tex->levels[0], su, sv, out_rgba);
        } else {
            sample_level_bilinear(&tex->levels[0], su, sv, out_rgba);
        }
        return;
    }

    // Minification
    switch (tex->min_filter) {
        case GL_NEAREST:
            sample_level_nearest(&tex->levels[0], su, sv, out_rgba);
            break;
        case GL_LINEAR:
            sample_level_bilinear(&tex->levels[0], su, sv, out_rgba);
            break;
        case GL_NEAREST_MIPMAP_NEAREST: {
            int d = (int)(lod + 0.5f);
            if (d < 0) d = 0;
            if (d > max_lvl) d = max_lvl;
            sample_level_nearest(&tex->levels[d], su, sv, out_rgba);
            break;
        }
        case GL_LINEAR_MIPMAP_NEAREST: {
            int d = (int)(lod + 0.5f);
            if (d < 0) d = 0;
            if (d > max_lvl) d = max_lvl;
            sample_level_bilinear(&tex->levels[d], su, sv, out_rgba);
            break;
        }
        case GL_NEAREST_MIPMAP_LINEAR: {
            int d1 = (int)lod;
            if (d1 < 0) d1 = 0;
            if (d1 > max_lvl) d1 = max_lvl;
            int d2 = d1 + 1;
            if (d2 > max_lvl) d2 = max_lvl;

            float c1[4], c2[4];
            sample_level_nearest(&tex->levels[d1], su, sv, c1);
            sample_level_nearest(&tex->levels[d2], su, sv, c2);

            float frac = lod - (float)d1;
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;

            for (int i = 0; i < 4; i++) {
                out_rgba[i] = (1.0f - frac) * c1[i] + frac * c2[i];
            }
            break;
        }
        case GL_LINEAR_MIPMAP_LINEAR: {
            int d1 = (int)lod;
            if (d1 < 0) d1 = 0;
            if (d1 > max_lvl) d1 = max_lvl;
            int d2 = d1 + 1;
            if (d2 > max_lvl) d2 = max_lvl;

            float c1[4], c2[4];
            sample_level_bilinear(&tex->levels[d1], su, sv, c1);
            sample_level_bilinear(&tex->levels[d2], su, sv, c2);

            float frac = lod - (float)d1;
            if (frac < 0.0f) frac = 0.0f;
            if (frac > 1.0f) frac = 1.0f;

            for (int i = 0; i < 4; i++) {
                out_rgba[i] = (1.0f - frac) * c1[i] + frac * c2[i];
            }
            break;
        }
        default:
            sample_level_nearest(&tex->levels[0], su, sv, out_rgba);
            break;
    }
}
