#include "kernel/graphics/gl/gl_pipeline.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_lighting.h"

uint32_t gl_pipeline_classify_clip(GLVec4 clip_pos) {
    uint32_t outcode = GL_OUTCODE_INSIDE;
    GLfloat w = clip_pos.w;

    if (w <= 0.0f) {
        // Invalid or behind camera
        outcode |= GL_OUTCODE_NEAR;
    }

    if (clip_pos.x < -w) outcode |= GL_OUTCODE_LEFT;
    if (clip_pos.x >  w) outcode |= GL_OUTCODE_RIGHT;
    if (clip_pos.y < -w) outcode |= GL_OUTCODE_BOTTOM;
    if (clip_pos.y >  w) outcode |= GL_OUTCODE_TOP;
    if (clip_pos.z < -w) outcode |= GL_OUTCODE_NEAR;
    if (clip_pos.z >  w) outcode |= GL_OUTCODE_FAR;

    return outcode;
}

void gl_pipeline_transform_vertex(GLVertex* v, const GLMatrix4x4* modelview, const GLMatrix4x4* projection) {
    if (!v || !modelview || !projection) return;

    // 1. Transform Object Space -> Eye Space
    v->eye_pos = gl_transform_point4(modelview, v->obj_pos);
    v->fog_coord = gl_fabsf(v->eye_pos.z);

    // 2. Transform Normal via Inverse-Transpose of ModelView 3x3
    float inv_trans[9];
    gl_matrix_inverse_transpose_3x3(inv_trans, modelview);

    float eye_norm[3];
    gl_transform_normal3(inv_trans, v->normal, eye_norm);

    GLContextState* state = gl_state_get_current();
    if (state && state->normalize_enabled) {
        gl_vec3_normalize(eye_norm);
    }

    // 3. Evaluate Fixed-Function Lighting in Eye Space (before projection & clipping)
    if (state && state->lighting_enabled) {
        float eye_p[4] = {v->eye_pos.x, v->eye_pos.y, v->eye_pos.z, v->eye_pos.w};
        gl_lighting_evaluate_vertex(state, eye_p, eye_norm, v->color, v->color);
    }

    // 4. Transform Eye Space -> Homogeneous Clip Space
    v->clip_pos = gl_transform_point4(projection, v->eye_pos);

    // 5. Classify homogeneous clip outcode
    v->clip_outcode = gl_pipeline_classify_clip(v->clip_pos);

    // 6. Safe inverse w
    if (gl_fabsf(v->clip_pos.w) > 1e-7f) {
        v->inv_w = 1.0f / v->clip_pos.w;
        v->is_valid = true;
    } else {
        v->inv_w = 0.0f;
        v->is_valid = false;
    }
}

uint32_t gl_pipeline_assemble_primitives(GLenum mode, const GLVertex* vertices, uint32_t count, GLPrimitive* out_primitives, uint32_t max_primitives) {
    if (!vertices || count == 0 || !out_primitives || max_primitives == 0) return 0;

    uint32_t prim_count = 0;

    switch (mode) {
        case GL_POINTS: {
            for (uint32_t i = 0; i < count && prim_count < max_primitives; i++) {
                out_primitives[prim_count].type = GL_POINTS;
                out_primitives[prim_count].vertex_count = 1;
                out_primitives[prim_count].vertices[0] = vertices[i];
                prim_count++;
            }
            break;
        }

        case GL_LINES: {
            for (uint32_t i = 0; i + 1 < count && prim_count < max_primitives; i += 2) {
                out_primitives[prim_count].type = GL_LINES;
                out_primitives[prim_count].vertex_count = 2;
                out_primitives[prim_count].vertices[0] = vertices[i];
                out_primitives[prim_count].vertices[1] = vertices[i + 1];
                prim_count++;
            }
            break;
        }

        case GL_LINE_STRIP: {
            for (uint32_t i = 0; i + 1 < count && prim_count < max_primitives; i++) {
                out_primitives[prim_count].type = GL_LINES;
                out_primitives[prim_count].vertex_count = 2;
                out_primitives[prim_count].vertices[0] = vertices[i];
                out_primitives[prim_count].vertices[1] = vertices[i + 1];
                prim_count++;
            }
            break;
        }

        case GL_LINE_LOOP: {
            for (uint32_t i = 0; i + 1 < count && prim_count < max_primitives; i++) {
                out_primitives[prim_count].type = GL_LINES;
                out_primitives[prim_count].vertex_count = 2;
                out_primitives[prim_count].vertices[0] = vertices[i];
                out_primitives[prim_count].vertices[1] = vertices[i + 1];
                prim_count++;
            }
            if (count > 2 && prim_count < max_primitives) {
                out_primitives[prim_count].type = GL_LINES;
                out_primitives[prim_count].vertex_count = 2;
                out_primitives[prim_count].vertices[0] = vertices[count - 1];
                out_primitives[prim_count].vertices[1] = vertices[0];
                prim_count++;
            }
            break;
        }

        case GL_TRIANGLES: {
            for (uint32_t i = 0; i + 2 < count && prim_count < max_primitives; i += 3) {
                out_primitives[prim_count].type = GL_TRIANGLES;
                out_primitives[prim_count].vertex_count = 3;
                out_primitives[prim_count].vertices[0] = vertices[i];
                out_primitives[prim_count].vertices[1] = vertices[i + 1];
                out_primitives[prim_count].vertices[2] = vertices[i + 2];
                prim_count++;
            }
            break;
        }

        case GL_TRIANGLE_STRIP: {
            for (uint32_t i = 0; i + 2 < count && prim_count < max_primitives; i++) {
                out_primitives[prim_count].type = GL_TRIANGLES;
                out_primitives[prim_count].vertex_count = 3;
                if (i % 2 == 0) {
                    out_primitives[prim_count].vertices[0] = vertices[i];
                    out_primitives[prim_count].vertices[1] = vertices[i + 1];
                    out_primitives[prim_count].vertices[2] = vertices[i + 2];
                } else {
                    out_primitives[prim_count].vertices[0] = vertices[i + 1];
                    out_primitives[prim_count].vertices[1] = vertices[i];
                    out_primitives[prim_count].vertices[2] = vertices[i + 2];
                }
                prim_count++;
            }
            break;
        }

        case GL_TRIANGLE_FAN: {
            for (uint32_t i = 1; i + 1 < count && prim_count < max_primitives; i++) {
                out_primitives[prim_count].type = GL_TRIANGLES;
                out_primitives[prim_count].vertex_count = 3;
                out_primitives[prim_count].vertices[0] = vertices[0];
                out_primitives[prim_count].vertices[1] = vertices[i];
                out_primitives[prim_count].vertices[2] = vertices[i + 1];
                prim_count++;
            }
            break;
        }

        case GL_QUADS: {
            for (uint32_t i = 0; i + 3 < count && prim_count + 1 < max_primitives; i += 4) {
                // First Triangle (0, 1, 2)
                out_primitives[prim_count].type = GL_TRIANGLES;
                out_primitives[prim_count].vertex_count = 3;
                out_primitives[prim_count].vertices[0] = vertices[i];
                out_primitives[prim_count].vertices[1] = vertices[i + 1];
                out_primitives[prim_count].vertices[2] = vertices[i + 2];
                prim_count++;

                // Second Triangle (0, 2, 3)
                out_primitives[prim_count].type = GL_TRIANGLES;
                out_primitives[prim_count].vertex_count = 3;
                out_primitives[prim_count].vertices[0] = vertices[i];
                out_primitives[prim_count].vertices[1] = vertices[i + 2];
                out_primitives[prim_count].vertices[2] = vertices[i + 3];
                prim_count++;
            }
            break;
        }

        default:
            break;
    }

    return prim_count;
}
