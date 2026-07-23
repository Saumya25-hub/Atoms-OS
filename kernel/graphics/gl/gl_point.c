#include "kernel/graphics/gl/gl_point.h"
#include "kernel/graphics/gl/gl_fragment.h"
#include "kernel/graphics/gl/gl_state.h"

void gl_rasterize_point(const GLScreenVertex* v, float size, const GLRenderTarget* target, GLContextState* state) {
    if (!v || !target || !state) return;
    if (size <= 0.0f) return;

    float half_size = size * 0.5f;
    int min_x = (int)(v->x - half_size);
    int max_x = (int)(v->x + half_size);
    int min_y = (int)(v->y - half_size);
    int max_y = (int)(v->y + half_size);

    if (size <= 1.0f) {
        min_x = (int)v->x; max_x = (int)v->x;
        min_y = (int)v->y; max_y = (int)v->y;
    }

    GLFragment frag;
    frag.depth = v->z;
    frag.color[0] = v->r;
    frag.color[1] = v->g;
    frag.color[2] = v->b;
    frag.color[3] = v->a;
    frag.has_texture = state->texture_2d_enabled;

    if (v->inv_w > 1e-7f) {
        frag.texcoord[0] = v->s_over_w / v->inv_w;
        frag.texcoord[1] = v->t_over_w / v->inv_w;
        frag.fog_coord   = v->fog_z_over_w / v->inv_w;
    } else {
        frag.texcoord[0] = 0.0f; frag.texcoord[1] = 0.0f;
        frag.fog_coord   = 0.0f;
    }
    frag.lod = 0.0f;

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            frag.x = x;
            frag.y = y;
            gl_fragment_process(state, target, &frag);
        }
    }
}
