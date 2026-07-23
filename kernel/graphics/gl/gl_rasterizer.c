#include "kernel/graphics/gl/gl_rasterizer.h"
#include "kernel/graphics/gl/gl_fragment.h"
#include "kernel/graphics/gl/gl_point.h"
#include "kernel/graphics/gl/gl_line.h"
#include "kernel/graphics/gl/gl_math.h"
#include "kernel/graphics/gl/gl_state.h"

static inline float edge_func(const GLScreenVertex* a, const GLScreenVertex* b, float x, float y) {
    return (x - a->x) * (b->y - a->y) - (y - a->y) * (b->x - a->x);
}

static inline bool is_top_left(const GLScreenVertex* a, const GLScreenVertex* b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    bool is_top  = (dy == 0.0f && dx > 0.0f);
    bool is_left = (dy < 0.0f);
    return is_top || is_left;
}

void gl_rasterize_triangle(const GLScreenVertex in_v[3], const GLRenderTarget* target, GLContextState* state) {
    if (!in_v || !target || !state) return;

    const GLScreenVertex* v0 = &in_v[0];
    const GLScreenVertex* v1 = &in_v[1];
    const GLScreenVertex* v2 = &in_v[2];

    // Compute signed area (2x area)
    float area = edge_func(v0, v1, v2->x, v2->y);
    if (gl_fabsf(area) < 1e-5f) return; // Degenerate triangle rejection

    // Face Culling Check
    bool is_ccw = (area > 0.0f);
    bool front_is_ccw = (state->front_face_mode == GL_CCW);
    bool is_front = (is_ccw == front_is_ccw);

    if (state->cull_face_enabled) {
        if (state->cull_face_mode == GL_FRONT_AND_BACK) {
            state->culled_triangles++;
            return;
        }
        if (state->cull_face_mode == GL_FRONT && is_front) {
            state->culled_triangles++;
            return;
        }
        if (state->cull_face_mode == GL_BACK && !is_front) {
            state->culled_triangles++;
            return;
        }
    }

    GLenum poly_mode = is_front ? state->polygon_mode_front : state->polygon_mode_back;

    // Handle Polygon Modes (GL_POINT, GL_LINE, GL_FILL)
    if (poly_mode == GL_POINT) {
        gl_rasterize_point(v0, state->point_size, target, state);
        gl_rasterize_point(v1, state->point_size, target, state);
        gl_rasterize_point(v2, state->point_size, target, state);
        return;
    } else if (poly_mode == GL_LINE) {
        gl_rasterize_line(v0, v1, state->line_width, target, state);
        gl_rasterize_line(v1, v2, state->line_width, target, state);
        gl_rasterize_line(v2, v0, state->line_width, target, state);
        return;
    }

    float inv_area = 1.0f / area;

    // Calculate Polygon Offset
    bool apply_offset = (poly_mode == GL_FILL && state->polygon_offset_fill_enabled);
    float poly_offset_val = 0.0f;
    if (apply_offset) {
        float dz_dx = gl_fabsf(((v1->y - v2->y) * v0->z + (v2->y - v0->y) * v1->z + (v0->y - v1->y) * v2->z) * inv_area);
        float dz_dy = gl_fabsf(((v2->x - v1->x) * v0->z + (v0->x - v2->x) * v1->z + (v1->x - v0->x) * v2->z) * inv_area);
        float max_m = (dz_dx > dz_dy) ? dz_dx : dz_dy;
        float r = 1e-7f;
        poly_offset_val = state->polygon_offset_factor * max_m + state->polygon_offset_units * r;
    }

    // Determine Bounding Box
    float min_x = v0->x; if (v1->x < min_x) min_x = v1->x; if (v2->x < min_x) min_x = v2->x;
    float max_x = v0->x; if (v1->x > max_x) max_x = v1->x; if (v2->x > max_x) max_x = v2->x;
    float min_y = v0->y; if (v1->y < min_y) min_y = v1->y; if (v2->y < min_y) min_y = v2->y;
    float max_y = v0->y; if (v1->y > max_y) max_y = v1->y; if (v2->y > max_y) max_y = v2->y;

    // Viewport & Render Target Bounding Box Clamping
    int vp_w = (state->viewport_w > 0) ? state->viewport_w : (int)target->width;
    int vp_h = (state->viewport_h > 0) ? state->viewport_h : (int)target->height;
    int clip_min_x = state->viewport_x;
    int clip_max_x = state->viewport_x + vp_w - 1;
    int clip_min_y = state->viewport_y;
    int clip_max_y = state->viewport_y + vp_h - 1;

    if (clip_min_x < 0) clip_min_x = 0;
    if (clip_max_x >= (int)target->width) clip_max_x = (int)target->width - 1;
    if (clip_min_y < 0) clip_min_y = 0;
    if (clip_max_y >= (int)target->height) clip_max_y = (int)target->height - 1;

    int ix_min = (int)min_x; if (ix_min < clip_min_x) ix_min = clip_min_x;
    int ix_max = (int)(max_x + 0.999f); if (ix_max > clip_max_x) ix_max = clip_max_x;
    int iy_min = (int)min_y; if (iy_min < clip_min_y) iy_min = clip_min_y;
    int iy_max = (int)(max_y + 0.999f); if (iy_max > clip_max_y) iy_max = clip_max_y;

    if (ix_min > ix_max || iy_min > iy_max) return;

    // Top-Left fill bias for shared edges
    float bias0 = is_top_left(v1, v2) ? 0.0f : -1e-4f;
    float bias1 = is_top_left(v2, v0) ? 0.0f : -1e-4f;
    float bias2 = is_top_left(v0, v1) ? 0.0f : -1e-4f;

    float dl0_dx = (v1->y - v2->y) * inv_area;
    float dl1_dx = (v2->y - v0->y) * inv_area;
    float dl2_dx = (v0->y - v1->y) * inv_area;

    float dl0_dy = (v2->x - v1->x) * inv_area;
    float dl1_dy = (v0->x - v2->x) * inv_area;
    float dl2_dy = (v1->x - v0->x) * inv_area;

    GLTextureObject* active_tex = NULL;
    if (state->texture_2d_enabled) {
        active_tex = gl_state_get_bound_texture(state);
    }

    state->rasterizer_triangles_submitted++;

    for (int y = iy_min; y <= iy_max; y++) {
        float py = (float)y + 0.5f;

        for (int x = ix_min; x <= ix_max; x++) {
            float px = (float)x + 0.5f;

            float w0 = edge_func(v1, v2, px, py) + bias0;
            float w1 = edge_func(v2, v0, px, py) + bias1;
            float w2 = edge_func(v0, v1, px, py) + bias2;

            bool inside = false;
            if (area > 0.0f) {
                inside = (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f);
            } else {
                inside = (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);
            }

            if (!inside) continue;

            state->rasterizer_fragments_tested++;

            // Barycentric coordinates
            float l0 = w0 * inv_area;
            float l1 = w1 * inv_area;
            float l2 = w2 * inv_area;

            // Interpolated Depth z
            float frag_z = l0 * v0->z + l1 * v1->z + l2 * v2->z;
            if (apply_offset) {
                frag_z += poly_offset_val;
                state->polygon_offset_fragments++;
            }
            if (frag_z < 0.0f) frag_z = 0.0f;
            if (frag_z > 1.0f) frag_z = 1.0f;

            // Interpolated RGB Vertex Color
            float r = l0 * v0->r + l1 * v1->r + l2 * v2->r;
            float g = l0 * v0->g + l1 * v1->g + l2 * v2->g;
            float b = l0 * v0->b + l1 * v1->b + l2 * v2->b;
            float a = l0 * v0->a + l1 * v1->a + l2 * v2->a;

            float s_interp = 0.0f, t_interp = 0.0f, fog_interp = 0.0f;

            // Hyperbolic Perspective-Correct Interpolation
            float denom = l0 * v0->inv_w + l1 * v1->inv_w + l2 * v2->inv_w;
            if (gl_fabsf(denom) > 1e-7f) {
                float inv_denom = 1.0f / denom;
                s_interp   = (l0 * v0->s_over_w + l1 * v1->s_over_w + l2 * v2->s_over_w) * inv_denom;
                t_interp   = (l0 * v0->t_over_w + l1 * v1->t_over_w + l2 * v2->t_over_w) * inv_denom;
                fog_interp = (l0 * v0->fog_z_over_w + l1 * v1->fog_z_over_w + l2 * v2->fog_z_over_w) * inv_denom;
            }

            float frag_lod = 0.0f;
            if (active_tex && active_tex->defined && active_tex->levels[0].defined && active_tex->levels[0].pixel_data) {
                float tw = (float)active_tex->levels[0].width;
                float th = (float)active_tex->levels[0].height;

                float l0_dx = l0 + dl0_dx, l1_dx = l1 + dl1_dx, l2_dx = l2 + dl2_dx;
                float denom_dx = l0_dx * v0->inv_w + l1_dx * v1->inv_w + l2_dx * v2->inv_w;
                float s_dx = s_interp, t_dx = t_interp;
                if (gl_fabsf(denom_dx) > 1e-7f) {
                    s_dx = (l0_dx * v0->s_over_w + l1_dx * v1->s_over_w + l2_dx * v2->s_over_w) / denom_dx;
                    t_dx = (l0_dx * v0->t_over_w + l1_dx * v1->t_over_w + l2_dx * v2->t_over_w) / denom_dx;
                }

                float l0_dy = l0 + dl0_dy, l1_dy = l1 + dl1_dy, l2_dy = l2 + dl2_dy;
                float denom_dy = l0_dy * v0->inv_w + l1_dy * v1->inv_w + l2_dy * v2->inv_w;
                float s_dy = s_interp, t_dy = t_interp;
                if (gl_fabsf(denom_dy) > 1e-7f) {
                    s_dy = (l0_dy * v0->s_over_w + l1_dy * v1->s_over_w + l2_dy * v2->s_over_w) / denom_dy;
                    t_dy = (l0_dy * v0->t_over_w + l1_dy * v1->t_over_w + l2_dy * v2->t_over_w) / denom_dy;
                }

                float ds_dx = (s_dx - s_interp) * tw;
                float dt_dx = (t_dx - t_interp) * th;
                float ds_dy = (s_dy - s_interp) * tw;
                float dt_dy = (t_dy - t_interp) * th;

                float rho_x_sq = ds_dx * ds_dx + dt_dx * dt_dx;
                float rho_y_sq = ds_dy * ds_dy + dt_dy * dt_dy;
                float max_rho_sq = (rho_x_sq > rho_y_sq) ? rho_x_sq : rho_y_sq;
                if (max_rho_sq > 1.0f) {
                    float rho = gl_sqrtf(max_rho_sq);
                    union { float f; uint32_t i; } u = { rho };
                    int exp = ((u.i >> 23) & 0xFF) - 127;
                    float mant = (float)(u.i & 0x007FFFFF) / (float)0x00800000;
                    frag_lod = (float)exp + mant * (1.442695f - 0.442695f * mant);
                    if (frag_lod < 0.0f) frag_lod = 0.0f;
                }
            }

            GLFragment frag;
            frag.x = x;
            frag.y = y;
            frag.depth = frag_z;
            frag.color[0] = r;
            frag.color[1] = g;
            frag.color[2] = b;
            frag.color[3] = a;
            frag.texcoord[0] = s_interp;
            frag.texcoord[1] = t_interp;
            frag.fog_coord   = fog_interp;
            frag.lod         = frag_lod;
            frag.has_texture = (active_tex != NULL);

            gl_fragment_process(state, target, &frag);
        }
    }
}
