#include "kernel/graphics/gl/gl_clip.h"
#include "kernel/graphics/gl/gl_math.h"

typedef struct {
    float nx, ny, nz, nw;
} GLClipPlane;

static const GLClipPlane s_clip_planes[6] = {
    { 1.0f,  0.0f,  0.0f, 1.0f}, // LEFT:   w + x >= 0
    {-1.0f,  0.0f,  0.0f, 1.0f}, // RIGHT:  w - x >= 0
    { 0.0f,  1.0f,  0.0f, 1.0f}, // BOTTOM: w + y >= 0
    { 0.0f, -1.0f,  0.0f, 1.0f}, // TOP:    w - y >= 0
    { 0.0f,  0.0f,  1.0f, 1.0f}, // NEAR:   w + z >= 0
    { 0.0f,  0.0f, -1.0f, 1.0f}  // FAR:    w - z >= 0
};

static float evaluate_plane_dist(const GLClipPlane* p, GLVec4 clip_pos) {
    return p->nx * clip_pos.x + p->ny * clip_pos.y + p->nz * clip_pos.z + p->nw * clip_pos.w;
}

static GLVertex interpolate_vertex(const GLVertex* v1, const GLVertex* v2, float t) {
    GLVertex out;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float omt = 1.0f - t;

    out.obj_pos.x  = omt * v1->obj_pos.x  + t * v2->obj_pos.x;
    out.obj_pos.y  = omt * v1->obj_pos.y  + t * v2->obj_pos.y;
    out.obj_pos.z  = omt * v1->obj_pos.z  + t * v2->obj_pos.z;
    out.obj_pos.w  = omt * v1->obj_pos.w  + t * v2->obj_pos.w;

    out.eye_pos.x  = omt * v1->eye_pos.x  + t * v2->eye_pos.x;
    out.eye_pos.y  = omt * v1->eye_pos.y  + t * v2->eye_pos.y;
    out.eye_pos.z  = omt * v1->eye_pos.z  + t * v2->eye_pos.z;
    out.eye_pos.w  = omt * v1->eye_pos.w  + t * v2->eye_pos.w;

    out.clip_pos.x = omt * v1->clip_pos.x + t * v2->clip_pos.x;
    out.clip_pos.y = omt * v1->clip_pos.y + t * v2->clip_pos.y;
    out.clip_pos.z = omt * v1->clip_pos.z + t * v2->clip_pos.z;
    out.clip_pos.w = omt * v1->clip_pos.w + t * v2->clip_pos.w;

    for (int i = 0; i < 4; i++) {
        out.color[i] = omt * v1->color[i] + t * v2->color[i];
        out.texcoord[i] = omt * v1->texcoord[i] + t * v2->texcoord[i];
    }

    for (int i = 0; i < 3; i++) {
        out.normal[i] = omt * v1->normal[i] + t * v2->normal[i];
    }

    out.fog_coord = omt * v1->fog_coord + t * v2->fog_coord;

    out.clip_outcode = gl_pipeline_classify_clip(out.clip_pos);

    if (gl_fabsf(out.clip_pos.w) > 1e-7f) {
        out.inv_w = 1.0f / out.clip_pos.w;
        out.is_valid = true;
    } else {
        out.inv_w = 0.0f;
        out.is_valid = false;
    }

    return out;
}

uint32_t gl_clip_triangle(const GLVertex in_tri[3], GLVertex out_polygon[GL_MAX_CLIPPED_POLYGON_VERTICES]) {
    if (!in_tri || !out_polygon) return 0;

    GLVertex poly_buf_a[GL_MAX_CLIPPED_POLYGON_VERTICES];
    GLVertex poly_buf_b[GL_MAX_CLIPPED_POLYGON_VERTICES];

    poly_buf_a[0] = in_tri[0];
    poly_buf_a[1] = in_tri[1];
    poly_buf_a[2] = in_tri[2];
    uint32_t count = 3;

    GLVertex* src = poly_buf_a;
    GLVertex* dst = poly_buf_b;

    // Sutherland-Hodgman clipping against 6 clip planes
    for (int plane_idx = 0; plane_idx < 6; plane_idx++) {
        if (count < 3) return 0; // Trivial reject

        const GLClipPlane* plane = &s_clip_planes[plane_idx];
        uint32_t out_count = 0;

        GLVertex prev_v = src[count - 1];
        float prev_dist = evaluate_plane_dist(plane, prev_v.clip_pos);

        for (uint32_t i = 0; i < count; i++) {
            GLVertex curr_v = src[i];
            float curr_dist = evaluate_plane_dist(plane, curr_v.clip_pos);

            if (curr_dist >= 0.0f) {
                if (prev_dist < 0.0f) {
                    float denom = (prev_dist - curr_dist);
                    float t = (gl_fabsf(denom) > 1e-7f) ? (prev_dist / denom) : 0.5f;
                    if (out_count < GL_MAX_CLIPPED_POLYGON_VERTICES) {
                        dst[out_count++] = interpolate_vertex(&prev_v, &curr_v, t);
                    }
                }
                if (out_count < GL_MAX_CLIPPED_POLYGON_VERTICES) {
                    dst[out_count++] = curr_v;
                }
            } else if (prev_dist >= 0.0f) {
                float denom = (prev_dist - curr_dist);
                float t = (gl_fabsf(denom) > 1e-7f) ? (prev_dist / denom) : 0.5f;
                if (out_count < GL_MAX_CLIPPED_POLYGON_VERTICES) {
                    dst[out_count++] = interpolate_vertex(&prev_v, &curr_v, t);
                }
            }

            prev_v = curr_v;
            prev_dist = curr_dist;
        }

        count = out_count;
        GLVertex* tmp = src;
        src = dst;
        dst = tmp;
    }

    for (uint32_t i = 0; i < count && i < GL_MAX_CLIPPED_POLYGON_VERTICES; i++) {
        out_polygon[i] = src[i];
    }

    return count;
}

uint32_t gl_triangulate_polygon(const GLVertex* polygon, uint32_t poly_vertex_count, GLPrimitive* out_triangles, uint32_t max_triangles) {
    if (!polygon || poly_vertex_count < 3 || !out_triangles || max_triangles == 0) return 0;

    uint32_t tri_count = 0;
    for (uint32_t i = 1; i + 1 < poly_vertex_count && tri_count < max_triangles; i++) {
        out_triangles[tri_count].type = GL_TRIANGLES;
        out_triangles[tri_count].vertex_count = 3;
        out_triangles[tri_count].vertices[0] = polygon[0];
        out_triangles[tri_count].vertices[1] = polygon[i];
        out_triangles[tri_count].vertices[2] = polygon[i + 1];
        tri_count++;
    }

    return tri_count;
}
