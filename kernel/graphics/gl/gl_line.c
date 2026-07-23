#include "kernel/graphics/gl/gl_line.h"
#include "kernel/graphics/gl/gl_fragment.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_math.h"

void gl_rasterize_line(const GLScreenVertex* v0, const GLScreenVertex* v1, float width, const GLRenderTarget* target, GLContextState* state) {
    if (!v0 || !v1 || !target || !state) return;
    if (width <= 0.0f) return;

    float dx = v1->x - v0->x;
    float dy = v1->y - v0->y;
    float steps = gl_fabsf(dx) > gl_fabsf(dy) ? gl_fabsf(dx) : gl_fabsf(dy);

    if (steps < 1.0f) steps = 1.0f;

    float x_inc = dx / steps;
    float y_inc = dy / steps;

    float cur_x = v0->x;
    float cur_y = v0->y;

    int half_w = (int)(width * 0.5f);
    if (width <= 1.0f) half_w = 0;

    GLFragment frag;
    frag.has_texture = state->texture_2d_enabled;

    for (int i = 0; i <= (int)steps; i++) {
        float t = (float)i / steps;
        float omt = 1.0f - t;

        frag.depth = omt * v0->z + t * v1->z;

        frag.color[0] = omt * v0->r + t * v1->r;
        frag.color[1] = omt * v0->g + t * v1->g;
        frag.color[2] = omt * v0->b + t * v1->b;
        frag.color[3] = omt * v0->a + t * v1->a;

        float inv_w_interp = omt * v0->inv_w + t * v1->inv_w;
        if (gl_fabsf(inv_w_interp) > 1e-7f) {
            float w_recip = 1.0f / inv_w_interp;
            float s_w = omt * v0->s_over_w + t * v1->s_over_w;
            float t_w = omt * v0->t_over_w + t * v1->t_over_w;
            float fog_w = omt * v0->fog_z_over_w + t * v1->fog_z_over_w;

            frag.texcoord[0] = s_w * w_recip;
            frag.texcoord[1] = t_w * w_recip;
            frag.fog_coord   = fog_w * w_recip;
        } else {
            frag.texcoord[0] = 0.0f; frag.texcoord[1] = 0.0f;
            frag.fog_coord   = 0.0f;
        }
        frag.lod = 0.0f;

        int px = (int)cur_x;
        int py = (int)cur_y;

        for (int wy = -half_w; wy <= half_w; wy++) {
            for (int wx = -half_w; wx <= half_w; wx++) {
                frag.x = px + wx;
                frag.y = py + wy;
                gl_fragment_process(state, target, &frag);
            }
        }

        cur_x += x_inc;
        cur_y += y_inc;
    }
}
