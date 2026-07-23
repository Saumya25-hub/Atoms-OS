#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_pipeline.h"
#include "kernel/graphics/gl/gl_clip.h"
#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/gl/gl_rasterizer.h"
#include "kernel/graphics/gl/gl_depth.h"
#include "kernel/graphics/gl/gl_texture.h"
#include "kernel/graphics/gl/gl_lighting.h"
#include "kernel/graphics/gl/gl_point.h"
#include "kernel/graphics/gl/gl_line.h"
#include "kernel/graphics/gl/gl_vertex_fetch.h"
#include "kernel/graphics/gl/gl_display_list.h"
#include "kernel/graphics/gl/gl_fbo.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/core/memory/heap/include/heap.h"

GLenum glGetError(void) {
    GLContextState* state = gl_state_get_current();
    if (!state) return GL_NO_ERROR;

    GLenum err = state->last_error;
    state->last_error = GL_NO_ERROR;
    return err;
}

void glEnable(GLenum cap) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (cap == GL_DEPTH_TEST) {
        state->depth_test_enabled = true;
    } else if (cap == GL_CULL_FACE) {
        state->cull_face_enabled = true;
    } else if (cap == GL_TEXTURE_2D) {
        state->texture_2d_enabled = true;
    } else if (cap == GL_LIGHTING) {
        state->lighting_enabled = true;
    } else if (cap == GL_NORMALIZE) {
        state->normalize_enabled = true;
    } else if (cap == GL_RESCALE_NORMAL) {
        state->rescale_normal_enabled = true;
    } else if (cap == GL_COLOR_MATERIAL) {
        state->color_material_enabled = true;
    } else if (cap == GL_ALPHA_TEST) {
        state->alpha_test_enabled = true;
    } else if (cap == GL_BLEND) {
        state->blend_enabled = true;
    } else if (cap == GL_FOG) {
        state->fog_enabled = true;
    } else if (cap == GL_SCISSOR_TEST) {
        state->scissor_test_enabled = true;
    } else if (cap == GL_STENCIL_TEST) {
        state->stencil_test_enabled = true;
    } else if (cap == GL_POLYGON_OFFSET_FILL) {
        state->polygon_offset_fill_enabled = true;
    } else if (cap == GL_POLYGON_OFFSET_LINE) {
        state->polygon_offset_line_enabled = true;
    } else if (cap == GL_POLYGON_OFFSET_POINT) {
        state->polygon_offset_point_enabled = true;
    } else if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + GL_MAX_LIGHTS) {
        GLuint idx = cap - GL_LIGHT0;
        state->lights[idx].enabled = true;
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
    }
}

void glDisable(GLenum cap) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (cap == GL_DEPTH_TEST) {
        state->depth_test_enabled = false;
    } else if (cap == GL_CULL_FACE) {
        state->cull_face_enabled = false;
    } else if (cap == GL_TEXTURE_2D) {
        state->texture_2d_enabled = false;
    } else if (cap == GL_LIGHTING) {
        state->lighting_enabled = false;
    } else if (cap == GL_NORMALIZE) {
        state->normalize_enabled = false;
    } else if (cap == GL_RESCALE_NORMAL) {
        state->rescale_normal_enabled = false;
    } else if (cap == GL_COLOR_MATERIAL) {
        state->color_material_enabled = false;
    } else if (cap == GL_ALPHA_TEST) {
        state->alpha_test_enabled = false;
    } else if (cap == GL_BLEND) {
        state->blend_enabled = false;
    } else if (cap == GL_FOG) {
        state->fog_enabled = false;
    } else if (cap == GL_SCISSOR_TEST) {
        state->scissor_test_enabled = false;
    } else if (cap == GL_STENCIL_TEST) {
        state->stencil_test_enabled = false;
    } else if (cap == GL_POLYGON_OFFSET_FILL) {
        state->polygon_offset_fill_enabled = false;
    } else if (cap == GL_POLYGON_OFFSET_LINE) {
        state->polygon_offset_line_enabled = false;
    } else if (cap == GL_POLYGON_OFFSET_POINT) {
        state->polygon_offset_point_enabled = false;
    } else if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + GL_MAX_LIGHTS) {
        GLuint idx = cap - GL_LIGHT0;
        state->lights[idx].enabled = false;
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
    }
}

GLboolean glIsEnabled(GLenum cap) {
    GLContextState* state = gl_state_get_current();
    if (!state) return GL_FALSE;

    if (cap == GL_DEPTH_TEST)          return state->depth_test_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_CULL_FACE)           return state->cull_face_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_TEXTURE_2D)          return state->texture_2d_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_LIGHTING)            return state->lighting_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_NORMALIZE)           return state->normalize_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_RESCALE_NORMAL)      return state->rescale_normal_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_COLOR_MATERIAL)      return state->color_material_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_ALPHA_TEST)          return state->alpha_test_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_BLEND)               return state->blend_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_FOG)                 return state->fog_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_SCISSOR_TEST)        return state->scissor_test_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_STENCIL_TEST)        return state->stencil_test_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_POLYGON_OFFSET_FILL)  return state->polygon_offset_fill_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_POLYGON_OFFSET_LINE)  return state->polygon_offset_line_enabled ? GL_TRUE : GL_FALSE;
    if (cap == GL_POLYGON_OFFSET_POINT) return state->polygon_offset_point_enabled ? GL_TRUE : GL_FALSE;
    if (cap >= GL_LIGHT0 && cap < GL_LIGHT0 + GL_MAX_LIGHTS) {
        GLuint idx = cap - GL_LIGHT0;
        return state->lights[idx].enabled ? GL_TRUE : GL_FALSE;
    }

    gl_state_set_error(state, GL_INVALID_ENUM);
    return GL_FALSE;
}

void glShadeModel(GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (mode != GL_FLAT && mode != GL_SMOOTH) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->shade_model = mode;
}

void glColorMaterial(GLenum face, GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (mode != GL_AMBIENT && mode != GL_DIFFUSE && mode != GL_SPECULAR && mode != GL_EMISSION && mode != GL_AMBIENT_AND_DIFFUSE) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->color_material_face = face;
    state->color_material_mode = mode;
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    state->current_normal[0] = nx;
    state->current_normal[1] = ny;
    state->current_normal[2] = nz;
}

void glNormal3fv(const GLfloat *v) {
    if (v) glNormal3f(v[0], v[1], v[2]);
}

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) {
    glNormal3f((GLfloat)nx, (GLfloat)ny, (GLfloat)nz);
}

void glNormal3dv(const GLdouble *v) {
    if (v) glNormal3f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (pname == GL_SHININESS) {
        if (param < 0.0f || param > 128.0f) {
            gl_state_set_error(state, GL_INVALID_VALUE);
            return;
        }
        if (face == GL_FRONT || face == GL_FRONT_AND_BACK) state->front_material.shininess = param;
        if (face == GL_BACK  || face == GL_FRONT_AND_BACK) state->back_material.shininess  = param;
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
    }
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    GLContextState* state = gl_state_get_current();
    if (!state || !params) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (pname == GL_SHININESS) {
        glMaterialf(face, pname, params[0]);
        return;
    }

    GLMaterialState* mats[2] = {NULL, NULL};
    int count = 0;
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK) mats[count++] = &state->front_material;
    if (face == GL_BACK  || face == GL_FRONT_AND_BACK) mats[count++] = &state->back_material;

    for (int i = 0; i < count; i++) {
        GLMaterialState* m = mats[i];
        switch (pname) {
            case GL_AMBIENT:
                m->ambient[0] = params[0]; m->ambient[1] = params[1]; m->ambient[2] = params[2]; m->ambient[3] = params[3];
                break;
            case GL_DIFFUSE:
                m->diffuse[0] = params[0]; m->diffuse[1] = params[1]; m->diffuse[2] = params[2]; m->diffuse[3] = params[3];
                break;
            case GL_SPECULAR:
                m->specular[0] = params[0]; m->specular[1] = params[1]; m->specular[2] = params[2]; m->specular[3] = params[3];
                break;
            case GL_EMISSION:
                m->emission[0] = params[0]; m->emission[1] = params[1]; m->emission[2] = params[2]; m->emission[3] = params[3];
                break;
            case GL_AMBIENT_AND_DIFFUSE:
                m->ambient[0] = params[0]; m->ambient[1] = params[1]; m->ambient[2] = params[2]; m->ambient[3] = params[3];
                m->diffuse[0] = params[0]; m->diffuse[1] = params[1]; m->diffuse[2] = params[2]; m->diffuse[3] = params[3];
                break;
            default:
                gl_state_set_error(state, GL_INVALID_ENUM);
                return;
        }
    }
}

void glLightf(GLenum light, GLenum pname, GLfloat param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (light < GL_LIGHT0 || light >= GL_LIGHT0 + GL_MAX_LIGHTS) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLuint idx = light - GL_LIGHT0;
    GLLightState* l = &state->lights[idx];

    switch (pname) {
        case GL_SPOT_EXPONENT:
            if (param < 0.0f || param > 128.0f) {
                gl_state_set_error(state, GL_INVALID_VALUE);
                return;
            }
            l->spot_exponent = param;
            break;
        case GL_SPOT_CUTOFF:
            if ((param < 0.0f || param > 90.0f) && param != 180.0f) {
                gl_state_set_error(state, GL_INVALID_VALUE);
                return;
            }
            l->spot_cutoff = param;
            break;
        case GL_CONSTANT_ATTENUATION:
            if (param < 0.0f) { gl_state_set_error(state, GL_INVALID_VALUE); return; }
            l->constant_attenuation = param;
            break;
        case GL_LINEAR_ATTENUATION:
            if (param < 0.0f) { gl_state_set_error(state, GL_INVALID_VALUE); return; }
            l->linear_attenuation = param;
            break;
        case GL_QUADRATIC_ATTENUATION:
            if (param < 0.0f) { gl_state_set_error(state, GL_INVALID_VALUE); return; }
            l->quadratic_attenuation = param;
            break;
        default:
            gl_state_set_error(state, GL_INVALID_ENUM);
            break;
    }
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
    GLContextState* state = gl_state_get_current();
    if (!state || !params) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (light < GL_LIGHT0 || light >= GL_LIGHT0 + GL_MAX_LIGHTS) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLuint idx = light - GL_LIGHT0;
    GLLightState* l = &state->lights[idx];

    if (pname == GL_SPOT_EXPONENT || pname == GL_SPOT_CUTOFF ||
        pname == GL_CONSTANT_ATTENUATION || pname == GL_LINEAR_ATTENUATION ||
        pname == GL_QUADRATIC_ATTENUATION) {
        glLightf(light, pname, params[0]);
        return;
    }

    switch (pname) {
        case GL_AMBIENT:
            l->ambient[0] = params[0]; l->ambient[1] = params[1]; l->ambient[2] = params[2]; l->ambient[3] = params[3];
            break;
        case GL_DIFFUSE:
            l->diffuse[0] = params[0]; l->diffuse[1] = params[1]; l->diffuse[2] = params[2]; l->diffuse[3] = params[3];
            break;
        case GL_SPECULAR:
            l->specular[0] = params[0]; l->specular[1] = params[1]; l->specular[2] = params[2]; l->specular[3] = params[3];
            break;
        case GL_POSITION: {
            // CRITICAL: Transform Light Position by CURRENT ModelView matrix at glLightfv time!
            GLMatrix4x4* modelview = gl_state_get_current_matrix(state);
            GLVec4 in_pos = {params[0], params[1], params[2], params[3]};
            GLVec4 eye_pos = gl_transform_point4(modelview, in_pos);
            l->position[0] = eye_pos.x;
            l->position[1] = eye_pos.y;
            l->position[2] = eye_pos.z;
            l->position[3] = eye_pos.w;
            break;
        }
        case GL_SPOT_DIRECTION: {
            // Transform Spot Direction by 3x3 Inverse Transpose of ModelView
            GLMatrix4x4* modelview = gl_state_get_current_matrix(state);
            float inv_trans[9];
            gl_matrix_inverse_transpose_3x3(inv_trans, modelview);
            float in_dir[3] = {params[0], params[1], params[2]};
            gl_transform_normal3(inv_trans, in_dir, l->spot_direction);
            gl_vec3_normalize(l->spot_direction);
            break;
        }
        default:
            gl_state_set_error(state, GL_INVALID_ENUM);
            break;
    }
}

void glLightModelf(GLenum pname, GLfloat param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (pname == GL_LIGHT_MODEL_LOCAL_VIEWER) {
        state->light_model_local_viewer = (param != 0.0f);
    } else if (pname == GL_LIGHT_MODEL_TWO_SIDE) {
        state->light_model_two_side = (param != 0.0f);
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
    }
}

void glLightModelfv(GLenum pname, const GLfloat *params) {
    GLContextState* state = gl_state_get_current();
    if (!state || !params) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (pname == GL_LIGHT_MODEL_AMBIENT) {
        state->light_model_ambient[0] = params[0];
        state->light_model_ambient[1] = params[1];
        state->light_model_ambient[2] = params[2];
        state->light_model_ambient[3] = params[3];
    } else if (pname == GL_LIGHT_MODEL_LOCAL_VIEWER || pname == GL_LIGHT_MODEL_TWO_SIDE) {
        glLightModelf(pname, params[0]);
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
    }
}

void glGenTextures(GLsizei n, GLuint *textures) {
    GLContextState* state = gl_state_get_current();
    if (!state || !textures) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        GLuint new_id = state->next_texture_id++;
        textures[i] = new_id;

        GLTextureObject* tex = NULL;
        for (size_t s = 1; s < GL_MAX_TEXTURE_OBJECTS; s++) {
            if (state->textures[s].id == 0) {
                tex = &state->textures[s];
                break;
            }
        }
        if (tex) {
            gl_texture_init_object(tex, new_id);
            tex->allocated = true;
        }
    }
}

void glDeleteTextures(GLsizei n, const GLuint *textures) {
    GLContextState* state = gl_state_get_current();
    if (!state || !textures) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        GLuint id = textures[i];
        if (id == 0) continue;

        if (state->bound_texture_2d == id) {
            state->bound_texture_2d = 0;
        }

        GLTextureObject* tex = gl_state_get_texture(state, id);
        if (tex) {
            gl_texture_free_object(tex);
            tex->id = 0;
        }

        gl_fbo_on_texture_deleted(state, id);
    }
}

void glBindTexture(GLenum target, GLuint texture) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (texture == 0) {
        state->bound_texture_2d = 0;
        return;
    }

    GLTextureObject* tex = gl_state_get_texture(state, texture);
    if (!tex) {
        for (size_t s = 1; s < GL_MAX_TEXTURE_OBJECTS; s++) {
            if (state->textures[s].id == 0) {
                tex = &state->textures[s];
                gl_texture_init_object(tex, texture);
                tex->allocated = true;
                break;
            }
        }
    }

    state->bound_texture_2d = texture;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_TEX_IMAGE_2D;
        cmd.data.tex_image_2d.level = level;
        cmd.data.tex_image_2d.internalformat = internalformat;
        cmd.data.tex_image_2d.width = width;
        cmd.data.tex_image_2d.height = height;
        cmd.data.tex_image_2d.border = border;
        cmd.data.tex_image_2d.format = format;
        cmd.data.tex_image_2d.type = type;
        size_t bpp = (format == GL_RGBA) ? 4 : 3;
        size_t unaligned_row = (size_t)width * bpp;
        size_t align = (size_t)state->unpack_alignment;
        size_t row_stride = (unaligned_row + align - 1) & ~(align - 1);
        size_t payload_sz = row_stride * (size_t)height;
        cmd.data.tex_image_2d.payload_size = payload_sz;
        if (pixels && payload_sz > 0) {
            cmd.data.tex_image_2d.pixel_payload = kmalloc(payload_sz);
            if (cmd.data.tex_image_2d.pixel_payload) {
                const uint8_t* src_p = (const uint8_t*)pixels;
                uint8_t* dst_p = (uint8_t*)cmd.data.tex_image_2d.pixel_payload;
                for (size_t i = 0; i < payload_sz; i++) dst_p[i] = src_p[i];
            }
        } else {
            cmd.data.tex_image_2d.pixel_payload = NULL;
        }
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    GLTextureObject* tex = gl_state_get_bound_texture(state);
    if (!tex) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (!gl_texture_upload_image(tex, level, internalformat, width, height, border, format, type, pixels, state->unpack_alignment)) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->texture_uploads++;
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_TEX_PARAMETERI;
        cmd.data.tex_param.pname = pname;
        cmd.data.tex_param.param = param;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    GLTextureObject* tex = gl_state_get_bound_texture(state);
    if (!tex) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (!gl_texture_set_parameter(tex, pname, param)) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_ENV || pname != GL_TEXTURE_ENV_MODE) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (param != GL_REPLACE && param != GL_MODULATE) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->texture_env_mode = (GLenum)param;
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    state->current_texcoord[0] = s;
    state->current_texcoord[1] = t;
    state->current_texcoord[2] = r;
    state->current_texcoord[3] = q;
}

void glTexCoord1f(GLfloat s) { glTexCoord4f(s, 0.0f, 0.0f, 1.0f); }
void glTexCoord2f(GLfloat s, GLfloat t) { glTexCoord4f(s, t, 0.0f, 1.0f); }
void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) { glTexCoord4f(s, t, r, 1.0f); }
void glTexCoord1fv(const GLfloat *v) { if (v) glTexCoord4f(v[0], 0.0f, 0.0f, 1.0f); }
void glTexCoord2fv(const GLfloat *v) { if (v) glTexCoord4f(v[0], v[1], 0.0f, 1.0f); }
void glTexCoord3fv(const GLfloat *v) { if (v) glTexCoord4f(v[0], v[1], v[2], 1.0f); }
void glTexCoord4fv(const GLfloat *v) { if (v) glTexCoord4f(v[0], v[1], v[2], v[3]); }

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (width < 0 || height < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->viewport_x = x;
    state->viewport_y = y;
    state->viewport_w = width;
    state->viewport_h = height;
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    state->clear_color[0] = (red < 0.0f) ? 0.0f : ((red > 1.0f) ? 1.0f : red);
    state->clear_color[1] = (green < 0.0f) ? 0.0f : ((green > 1.0f) ? 1.0f : green);
    state->clear_color[2] = (blue < 0.0f) ? 0.0f : ((blue > 1.0f) ? 1.0f : blue);
    state->clear_color[3] = (alpha < 0.0f) ? 0.0f : ((alpha > 1.0f) ? 1.0f : alpha);
}

void glClearDepth(GLclampf depth) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    state->clear_depth = (depth < 0.0f) ? 0.0f : ((depth > 1.0f) ? 1.0f : depth);
}

void glClear(GLbitfield mask) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if ((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    GLRenderTarget target;
    if (!gl_get_active_render_target(state, &target)) return;

    uint32_t w = target.width;
    uint32_t h = target.height;
    if (w == 0 || h == 0) return;

    int min_x = 0, max_x = (int)w - 1;
    int min_y = 0, max_y = (int)h - 1;

    if (state->scissor_test_enabled) {
        int top_y    = (int)h - (state->scissor_y + state->scissor_h);
        int bottom_y = (int)h - state->scissor_y;
        min_x = state->scissor_x;
        max_x = state->scissor_x + state->scissor_w - 1;
        min_y = top_y;
        max_y = bottom_y - 1;

        if (min_x < 0) min_x = 0;
        if (max_x >= (int)w) max_x = (int)w - 1;
        if (min_y < 0) min_y = 0;
        if (max_y >= (int)h) max_y = (int)h - 1;

        if (min_x > max_x || min_y > max_y) return;
    }

    bool full_clear = (min_x == 0 && max_x == (int)w - 1 && min_y == 0 && max_y == (int)h - 1);

    if (mask & GL_COLOR_BUFFER_BIT) {
        if (target.has_color && target.color_buffer) {
            uint32_t r = (uint32_t)(state->clear_color[0] * 255.0f);
            uint32_t g = (uint32_t)(state->clear_color[1] * 255.0f);
            uint32_t b = (uint32_t)(state->clear_color[2] * 255.0f);
            uint32_t a = (uint32_t)(state->clear_color[3] * 255.0f);
            uint32_t argb = (a << 24) | (r << 16) | (g << 8) | b;

            BGLContext* bgl_ctx = bglGetCurrentContext();
            if (full_clear && !target.is_fbo && bgl_ctx) {
                bglDiagnosticClear(bgl_ctx, argb);
            } else {
                for (int y = min_y; y <= max_y; y++) {
                    uint32_t row_start = (uint32_t)y * target.color_pitch;
                    for (int x = min_x; x <= max_x; x++) {
                        target.color_buffer[row_start + (uint32_t)x] = argb;
                    }
                }
                if (!target.is_fbo && bgl_ctx && bgl_ctx->bound_drawable) {
                    bgl_ctx->bound_drawable->is_dirty = true;
                }
            }
        }
    }

    if (mask & GL_DEPTH_BUFFER_BIT) {
        if (target.has_depth && target.depth_buffer) {
            if (full_clear) {
                gl_depth_clear(&target, state->clear_depth);
            } else {
                for (int y = min_y; y <= max_y; y++) {
                    uint32_t row_start = (uint32_t)y * target.depth_pitch;
                    for (int x = min_x; x <= max_x; x++) {
                        target.depth_buffer[row_start + (uint32_t)x] = state->clear_depth;
                    }
                }
            }
        }
    }

    if (mask & GL_STENCIL_BUFFER_BIT) {
        if (target.has_stencil && target.stencil_buffer) {
            uint8_t clear_s = (uint8_t)(state->clear_stencil & 0xFF);
            if (full_clear) {
                size_t total = (size_t)w * (size_t)h;
                for (size_t i = 0; i < total; i++) {
                    target.stencil_buffer[i] = clear_s;
                }
            } else {
                for (int y = min_y; y <= max_y; y++) {
                    uint32_t row_start = (uint32_t)y * target.stencil_pitch;
                    for (int x = min_x; x <= max_x; x++) {
                        target.stencil_buffer[row_start + (uint32_t)x] = clear_s;
                    }
                }
            }
        }
    }
}

void glDepthFunc(GLenum func) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (func < GL_NEVER || func > GL_ALWAYS) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->depth_func = func;
}

void glDepthMask(GLboolean flag) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_DEPTH_MASK;
        cmd.data.depth_mask.flag = flag;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->depth_writemask = (flag != GL_FALSE);
}

void glCullFace(GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_CULL_FACE;
        cmd.data.cull_face.mode = mode;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->cull_face_mode = mode;
}

void glFrontFace(GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (mode != GL_CW && mode != GL_CCW) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_FRONT_FACE;
        cmd.data.front_face.mode = mode;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->front_face_mode = mode;
}

void glMatrixMode(GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (mode != GL_MODELVIEW && mode != GL_PROJECTION) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->matrix_mode = mode;
}

void glLoadIdentity(void) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_identity(M);
}

void glLoadMatrixf(const GLfloat *m) {
    GLContextState* state = gl_state_get_current();
    if (!state || !m) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    for (int i = 0; i < 16; i++) {
        M->m[i] = m[i];
    }
}

void glMultMatrixf(const GLfloat *m) {
    GLContextState* state = gl_state_get_current();
    if (!state || !m) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4 B;
    for (int i = 0; i < 16; i++) B.m[i] = m[i];

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_multiply(M, M, &B);
}

void glPushMatrix(void) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->matrix_mode == GL_PROJECTION) {
        if (state->projection_top + 1 >= GL_MAX_PROJECTION_STACK_DEPTH) {
            gl_state_set_error(state, GL_STACK_OVERFLOW);
            return;
        }
        state->projection_stack[state->projection_top + 1] = state->projection_stack[state->projection_top];
        state->projection_top++;
    } else {
        if (state->modelview_top + 1 >= GL_MAX_MODELVIEW_STACK_DEPTH) {
            gl_state_set_error(state, GL_STACK_OVERFLOW);
            return;
        }
        state->modelview_stack[state->modelview_top + 1] = state->modelview_stack[state->modelview_top];
        state->modelview_top++;
    }
}

void glPopMatrix(void) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->matrix_mode == GL_PROJECTION) {
        if (state->projection_top <= 0) {
            gl_state_set_error(state, GL_STACK_UNDERFLOW);
            return;
        }
        state->projection_top--;
    } else {
        if (state->modelview_top <= 0) {
            gl_state_set_error(state, GL_STACK_UNDERFLOW);
            return;
        }
        state->modelview_top--;
    }
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_translate(M, M, x, y, z);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_rotate(M, M, angle, x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_scale(M, M, x, y, z);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_ortho(M, (GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)zNear, (GLfloat)zFar);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    gl_matrix_frustum(M, (GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)zNear, (GLfloat)zFar);
}

void glBegin(GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (mode != GL_POINTS && mode != GL_LINES && mode != GL_LINE_STRIP && 
        mode != GL_LINE_LOOP && mode != GL_TRIANGLES && mode != GL_TRIANGLE_STRIP && 
        mode != GL_TRIANGLE_FAN && mode != GL_QUADS && mode != GL_QUAD_STRIP && mode != GL_POLYGON) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_BEGIN;
        cmd.data.begin.mode = mode;
        gl_display_list_record_command(state, &cmd);
        state->in_begin_end = true;
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->in_begin_end = true;
    state->primitive_mode = mode;
    state->vertex_count = 0;
}

void glEnd(void) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (!state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_END;
        gl_display_list_record_command(state, &cmd);
        state->in_begin_end = false;
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->in_begin_end = false;

    if (state->vertex_count == 0) return;

    // Assemble primitives from accumulated vertices
    uint32_t needed_prims = state->vertex_count * 2;
    if (needed_prims > state->primitive_capacity) {
        GLPrimitive* new_buf = (GLPrimitive*)kmalloc(needed_prims * sizeof(GLPrimitive));
        if (new_buf) {
            for (uint32_t i = 0; i < state->primitive_count; i++) new_buf[i] = state->primitive_buffer[i];
            kfree(state->primitive_buffer);
            state->primitive_buffer = new_buf;
            state->primitive_capacity = needed_prims;
        }
    }

    uint32_t prims_assembled = gl_pipeline_assemble_primitives(
        state->primitive_mode,
        state->vertex_buffer,
        state->vertex_count,
        state->primitive_buffer,
        state->primitive_capacity
    );

    state->primitive_count = prims_assembled;
    state->total_primitives_assembled += prims_assembled;

    // Dispatch assembled triangle primitives into Software Rasterization Pipeline
    GLRenderTarget target;
    if (!gl_get_active_render_target(state, &target)) return;

    if (target.is_fbo) {
        state->render_to_texture_passes++;
    }

    for (uint32_t i = 0; i < prims_assembled; i++) {
        GLPrimitive* prim = &state->primitive_buffer[i];

        if (prim->type == GL_POINTS && prim->vertex_count >= 1) {
            GLVertex v = prim->vertices[0];
            if ((v.clip_outcode & 0x3F) == 0) { // inside view frustum
                GLScreenVertex sv;
                if (gl_viewport_transform_vertex(&v, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv)) {
                    gl_rasterize_point(&sv, state->point_size, &target, state);
                }
            }
        } else if (prim->type == GL_LINES && prim->vertex_count >= 2) {
            GLVertex v0 = prim->vertices[0];
            GLVertex v1 = prim->vertices[1];
            if ((v0.clip_outcode & v1.clip_outcode & 0x3F) == 0) { // Not trivially rejected
                GLScreenVertex sv0, sv1;
                bool ok0 = gl_viewport_transform_vertex(&v0, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv0);
                bool ok1 = gl_viewport_transform_vertex(&v1, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv1);
                if (ok0 && ok1) {
                    gl_rasterize_line(&sv0, &sv1, state->line_width, &target, state);
                }
            }
        } else if (prim->type == GL_TRIANGLES && prim->vertex_count >= 3) {
            // GL_FLAT Shading Model: Provoking vertex (last vertex) color assigned to all triangle vertices
            if (state->shade_model == GL_FLAT) {
                prim->vertices[0].color[0] = prim->vertices[2].color[0];
                prim->vertices[0].color[1] = prim->vertices[2].color[1];
                prim->vertices[0].color[2] = prim->vertices[2].color[2];
                prim->vertices[0].color[3] = prim->vertices[2].color[3];

                prim->vertices[1].color[0] = prim->vertices[2].color[0];
                prim->vertices[1].color[1] = prim->vertices[2].color[1];
                prim->vertices[1].color[2] = prim->vertices[2].color[2];
                prim->vertices[1].color[3] = prim->vertices[2].color[3];
            }

            // 1. Homogeneous 4D Clipping (against 6 clip planes with attribute interpolation)
            GLVertex clipped_poly[GL_MAX_CLIPPED_POLYGON_VERTICES];
            uint32_t poly_count = gl_clip_triangle(prim->vertices, clipped_poly);
            if (poly_count < 3) continue; // Fully clipped out

            // 2. Triangulate clipped polygon
            GLPrimitive clipped_tris[GL_MAX_CLIPPED_POLYGON_VERTICES];
            uint32_t tri_count = gl_triangulate_polygon(clipped_poly, poly_count, clipped_tris, GL_MAX_CLIPPED_POLYGON_VERTICES);

            // 3. For each triangle: Perspective Divide -> Viewport Transform -> Rasterize
            for (uint32_t t = 0; t < tri_count; t++) {
                GLScreenVertex screen_v[3];
                bool v0_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[0], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[0]);
                bool v1_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[1], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[1]);
                bool v2_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[2], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[2]);

                if (v0_ok && v1_ok && v2_ok) {
                    gl_rasterize_triangle(screen_v, &target, state);
                }
            }
        }
    }
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_VERTEX4F;
        cmd.data.vertex.x = x; cmd.data.vertex.y = y; cmd.data.vertex.z = z; cmd.data.vertex.w = w;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    if (!state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->vertex_count >= state->vertex_capacity) {
        uint32_t new_cap = state->vertex_capacity * 2;
        if (new_cap > GL_MAX_VERTICES_PER_BEGIN) {
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        GLVertex* new_buf = (GLVertex*)kmalloc(new_cap * sizeof(GLVertex));
        if (!new_buf) {
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        for (uint32_t i = 0; i < state->vertex_count; i++) new_buf[i] = state->vertex_buffer[i];
        kfree(state->vertex_buffer);
        state->vertex_buffer = new_buf;
        state->vertex_capacity = new_cap;
    }

    GLVertex* v = &state->vertex_buffer[state->vertex_count];
    v->obj_pos.x = x;
    v->obj_pos.y = y;
    v->obj_pos.z = z;
    v->obj_pos.w = w;

    v->color[0] = state->current_color[0];
    v->color[1] = state->current_color[1];
    v->color[2] = state->current_color[2];
    v->color[3] = state->current_color[3];

    v->texcoord[0] = state->current_texcoord[0];
    v->texcoord[1] = state->current_texcoord[1];
    v->texcoord[2] = state->current_texcoord[2];
    v->texcoord[3] = state->current_texcoord[3];

    v->normal[0] = state->current_normal[0];
    v->normal[1] = state->current_normal[1];
    v->normal[2] = state->current_normal[2];

    GLMatrix4x4* modelview = &state->modelview_stack[state->modelview_top];
    GLMatrix4x4* projection = &state->projection_stack[state->projection_top];
    gl_pipeline_transform_vertex(v, modelview, projection);

    state->vertex_count++;
    state->total_vertices_transformed++;
}

void glVertex2f(GLfloat x, GLfloat y) { glVertex4f(x, y, 0.0f, 1.0f); }
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) { glVertex4f(x, y, z, 1.0f); }
void glVertex2fv(const GLfloat *v) { if (v) glVertex4f(v[0], v[1], 0.0f, 1.0f); }
void glVertex3fv(const GLfloat *v) { if (v) glVertex4f(v[0], v[1], v[2], 1.0f); }
void glVertex4fv(const GLfloat *v) { if (v) glVertex4f(v[0], v[1], v[2], v[3]); }

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_COLOR4F;
        cmd.data.color.r = red; cmd.data.color.g = green; cmd.data.color.b = blue; cmd.data.color.a = alpha;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->current_color[0] = (red < 0.0f) ? 0.0f : ((red > 1.0f) ? 1.0f : red);
    state->current_color[1] = (green < 0.0f) ? 0.0f : ((green > 1.0f) ? 1.0f : green);
    state->current_color[2] = (blue < 0.0f) ? 0.0f : ((blue > 1.0f) ? 1.0f : blue);
    state->current_color[3] = (alpha < 0.0f) ? 0.0f : ((alpha > 1.0f) ? 1.0f : alpha);

    if (state->color_material_enabled) {
        GLfloat c[4] = {state->current_color[0], state->current_color[1], state->current_color[2], state->current_color[3]};
        glMaterialfv(state->color_material_face, state->color_material_mode, c);
    }
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) { glColor4f(red, green, blue, 1.0f); }
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) { glColor4f((GLfloat)red / 255.0f, (GLfloat)green / 255.0f, (GLfloat)blue / 255.0f, 1.0f); }
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) { glColor4f((GLfloat)red / 255.0f, (GLfloat)green / 255.0f, (GLfloat)blue / 255.0f, (GLfloat)alpha / 255.0f); }
void glColor4fv(const GLfloat *v) { if (v) glColor4f(v[0], v[1], v[2], v[3]); }

/* --- Phase 6 Fragment & Rasterization APIs --- */

void glAlphaFunc(GLenum func, GLclampf ref) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (func < GL_NEVER || func > GL_ALWAYS) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->alpha_func = func;
    state->alpha_ref  = (ref < 0.0f) ? 0.0f : ((ref > 1.0f) ? 1.0f : ref);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    bool s_ok = (sfactor == GL_ZERO || sfactor == GL_ONE || sfactor == GL_SRC_COLOR ||
                 sfactor == GL_ONE_MINUS_SRC_COLOR || sfactor == GL_SRC_ALPHA ||
                 sfactor == GL_ONE_MINUS_SRC_ALPHA || sfactor == GL_DST_ALPHA ||
                 sfactor == GL_ONE_MINUS_DST_ALPHA || sfactor == GL_DST_COLOR ||
                 sfactor == GL_ONE_MINUS_DST_COLOR || sfactor == GL_SRC_ALPHA_SATURATE);

    bool d_ok = (dfactor == GL_ZERO || dfactor == GL_ONE || dfactor == GL_SRC_COLOR ||
                 dfactor == GL_ONE_MINUS_SRC_COLOR || dfactor == GL_SRC_ALPHA ||
                 dfactor == GL_ONE_MINUS_SRC_ALPHA || dfactor == GL_DST_ALPHA ||
                 dfactor == GL_ONE_MINUS_DST_ALPHA || dfactor == GL_DST_COLOR ||
                 dfactor == GL_ONE_MINUS_DST_COLOR);

    if (!s_ok || !d_ok) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    state->blend_src_factor = sfactor;
    state->blend_dst_factor = dfactor;
}

void glFogf(GLenum pname, GLfloat param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (pname == GL_FOG_MODE) {
        GLenum mode = (GLenum)param;
        if (mode == GL_LINEAR || mode == GL_EXP || mode == GL_EXP2) {
            state->fog_mode = mode;
        } else {
            gl_state_set_error(state, GL_INVALID_ENUM);
        }
    } else if (pname == GL_FOG_DENSITY) {
        if (param < 0.0f) {
            gl_state_set_error(state, GL_INVALID_VALUE);
        } else {
            state->fog_density = param;
        }
    } else if (pname == GL_FOG_START) {
        state->fog_start = param;
    } else if (pname == GL_FOG_END) {
        state->fog_end = param;
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
    }
}

void glFogfv(GLenum pname, const GLfloat *params) {
    GLContextState* state = gl_state_get_current();
    if (!state || !params) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (pname == GL_FOG_COLOR) {
        for (int i = 0; i < 4; i++) {
            float c = params[i];
            state->fog_color[i] = (c < 0.0f) ? 0.0f : ((c > 1.0f) ? 1.0f : c);
        }
    } else {
        glFogf(pname, params[0]);
    }
}

void glFogi(GLenum pname, GLint param) { glFogf(pname, (GLfloat)param); }
void glFogiv(GLenum pname, const GLint *params) {
    if (!params) return;
    if (pname == GL_FOG_COLOR) {
        GLfloat f[4] = {(GLfloat)params[0], (GLfloat)params[1], (GLfloat)params[2], (GLfloat)params[3]};
        glFogfv(pname, f);
    } else {
        glFogf(pname, (GLfloat)params[0]);
    }
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (width < 0 || height < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->scissor_x = x;
    state->scissor_y = y;
    state->scissor_w = width;
    state->scissor_h = height;
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    state->color_mask[0] = (red != GL_FALSE);
    state->color_mask[1] = (green != GL_FALSE);
    state->color_mask[2] = (blue != GL_FALSE);
    state->color_mask[3] = (alpha != GL_FALSE);
}

void glDepthRange(GLclampd nearVal, GLclampd farVal) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    state->depth_range_near = (nearVal < 0.0) ? 0.0 : ((nearVal > 1.0) ? 1.0 : nearVal);
    state->depth_range_far  = (farVal < 0.0) ? 0.0 : ((farVal > 1.0) ? 1.0 : farVal);
}

void glPointSize(GLfloat size) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (size <= 0.0f) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->point_size = size;
}

void glLineWidth(GLfloat width) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (width <= 0.0f) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->line_width = width;
}

void glPolygonMode(GLenum face, GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
        state->polygon_mode_front = mode;
    }
    if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
        state->polygon_mode_back = mode;
    }
}

/* --- Client Array Operations --- */

void glEnableClientState(GLenum cap) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;
    switch (cap) {
        case GL_VERTEX_ARRAY:        state->vertex_array.enabled = true; break;
        case GL_COLOR_ARRAY:         state->color_array.enabled = true; break;
        case GL_NORMAL_ARRAY:        state->normal_array.enabled = true; break;
        case GL_TEXTURE_COORD_ARRAY: state->texcoord_array.enabled = true; break;
        default: gl_state_set_error(state, GL_INVALID_ENUM); break;
    }
}

void glDisableClientState(GLenum cap) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;
    switch (cap) {
        case GL_VERTEX_ARRAY:        state->vertex_array.enabled = false; break;
        case GL_COLOR_ARRAY:         state->color_array.enabled = false; break;
        case GL_NORMAL_ARRAY:        state->normal_array.enabled = false; break;
        case GL_TEXTURE_COORD_ARRAY: state->texcoord_array.enabled = false; break;
        default: gl_state_set_error(state, GL_INVALID_ENUM); break;
    }
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;
    if (size < 2 || size > 4 || stride < 0 || !pointer) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }
    state->vertex_array.size = size;
    state->vertex_array.type = type;
    state->vertex_array.stride = stride;
    state->vertex_array.pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;
    if (size < 3 || size > 4 || stride < 0 || !pointer) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }
    state->color_array.size = size;
    state->color_array.type = type;
    state->color_array.stride = stride;
    state->color_array.pointer = pointer;
}

void glNormalPointer(GLenum type, GLsizei stride, const void* pointer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;
    if (stride < 0 || !pointer) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }
    state->normal_array.size = 3;
    state->normal_array.type = type;
    state->normal_array.stride = stride;
    state->normal_array.pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;
    if (size < 1 || size > 4 || stride < 0 || !pointer) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }
    state->texcoord_array.size = size;
    state->texcoord_array.type = type;
    state->texcoord_array.stride = stride;
    state->texcoord_array.pointer = pointer;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (first < 0 || count < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    if (count == 0) return;

    if (mode < GL_POINTS || mode > GL_POLYGON) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (!state->vertex_array.enabled || !state->vertex_array.pointer) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLRenderTarget target;
    if (!gl_get_active_render_target(state, &target)) return;

    state->draw_calls++;
    state->vertices_submitted += (uint64_t)count;

    GLVertex stack_vertices[256];
    GLVertex* v_buf = stack_vertices;
    bool free_v_buf = false;
    if ((uint32_t)count > 256) {
        v_buf = (GLVertex*)kmalloc((size_t)count * sizeof(GLVertex));
        if (!v_buf) {
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        free_v_buf = true;
    }

    for (GLsizei i = 0; i < count; i++) {
        gl_fetch_client_vertex(state, (uint32_t)(first + i), &v_buf[i]);
    }

    uint32_t needed_prims = (uint32_t)count * 2;
    GLPrimitive stack_prims[256];
    GLPrimitive* p_buf = stack_prims;
    bool free_p_buf = false;
    if (needed_prims > 256) {
        p_buf = (GLPrimitive*)kmalloc(needed_prims * sizeof(GLPrimitive));
        if (!p_buf) {
            if (free_v_buf) kfree(v_buf);
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        free_p_buf = true;
    }

    uint32_t prims_assembled = gl_pipeline_assemble_primitives(mode, v_buf, (uint32_t)count, p_buf, needed_prims);

    for (uint32_t i = 0; i < prims_assembled; i++) {
        GLPrimitive* prim = &p_buf[i];
        if (prim->type == GL_POINTS && prim->vertex_count >= 1) {
            GLVertex v = prim->vertices[0];
            if ((v.clip_outcode & 0x3F) == 0) {
                GLScreenVertex sv;
                if (gl_viewport_transform_vertex(&v, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv)) {
                    gl_rasterize_point(&sv, state->point_size, &target, state);
                }
            }
        } else if (prim->type == GL_LINES && prim->vertex_count >= 2) {
            GLVertex v0 = prim->vertices[0];
            GLVertex v1 = prim->vertices[1];
            if ((v0.clip_outcode & v1.clip_outcode & 0x3F) == 0) {
                GLScreenVertex sv0, sv1;
                bool ok0 = gl_viewport_transform_vertex(&v0, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv0);
                bool ok1 = gl_viewport_transform_vertex(&v1, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv1);
                if (ok0 && ok1) {
                    gl_rasterize_line(&sv0, &sv1, state->line_width, &target, state);
                }
            }
        } else if (prim->type == GL_TRIANGLES && prim->vertex_count >= 3) {
            if (state->shade_model == GL_FLAT) {
                prim->vertices[0].color[0] = prim->vertices[2].color[0];
                prim->vertices[0].color[1] = prim->vertices[2].color[1];
                prim->vertices[0].color[2] = prim->vertices[2].color[2];
                prim->vertices[0].color[3] = prim->vertices[2].color[3];
                prim->vertices[1].color[0] = prim->vertices[2].color[0];
                prim->vertices[1].color[1] = prim->vertices[2].color[1];
                prim->vertices[1].color[2] = prim->vertices[2].color[2];
                prim->vertices[1].color[3] = prim->vertices[2].color[3];
            }
            GLVertex clipped_poly[GL_MAX_CLIPPED_POLYGON_VERTICES];
            uint32_t poly_count = gl_clip_triangle(prim->vertices, clipped_poly);
            if (poly_count < 3) continue;
            GLPrimitive clipped_tris[GL_MAX_CLIPPED_POLYGON_VERTICES];
            uint32_t tri_count = gl_triangulate_polygon(clipped_poly, poly_count, clipped_tris, GL_MAX_CLIPPED_POLYGON_VERTICES);
            for (uint32_t t = 0; t < tri_count; t++) {
                GLScreenVertex screen_v[3];
                bool v0_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[0], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[0]);
                bool v1_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[1], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[1]);
                bool v2_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[2], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[2]);
                if (v0_ok && v1_ok && v2_ok) {
                    gl_rasterize_triangle(screen_v, &target, state);
                }
            }
        }
    }

    if (free_v_buf) kfree(v_buf);
    if (free_p_buf) kfree(p_buf);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (count < 0 || !indices) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    if (count == 0) return;

    if (mode < GL_POINTS || mode > GL_POLYGON) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (!state->vertex_array.enabled || !state->vertex_array.pointer) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLRenderTarget target;
    if (!gl_get_active_render_target(state, &target)) return;

    state->draw_calls++;
    state->vertices_submitted += (uint64_t)count;
    state->indices_processed += (uint64_t)count;

    GLVertex stack_vertices[256];
    GLVertex* v_buf = stack_vertices;
    bool free_v_buf = false;
    if ((uint32_t)count > 256) {
        v_buf = (GLVertex*)kmalloc((size_t)count * sizeof(GLVertex));
        if (!v_buf) {
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        free_v_buf = true;
    }

    for (GLsizei i = 0; i < count; i++) {
        uint32_t idx = 0;
        if (type == GL_UNSIGNED_BYTE) {
            idx = (uint32_t)(((const uint8_t*)indices)[i]);
        } else if (type == GL_UNSIGNED_SHORT) {
            idx = (uint32_t)(((const uint16_t*)indices)[i]);
        } else if (type == GL_UNSIGNED_INT) {
            idx = ((const uint32_t*)indices)[i];
        }
        gl_fetch_client_vertex(state, idx, &v_buf[i]);
    }

    uint32_t needed_prims = (uint32_t)count * 2;
    GLPrimitive stack_prims[256];
    GLPrimitive* p_buf = stack_prims;
    bool free_p_buf = false;
    if (needed_prims > 256) {
        p_buf = (GLPrimitive*)kmalloc(needed_prims * sizeof(GLPrimitive));
        if (!p_buf) {
            if (free_v_buf) kfree(v_buf);
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        free_p_buf = true;
    }

    uint32_t prims_assembled = gl_pipeline_assemble_primitives(mode, v_buf, (uint32_t)count, p_buf, needed_prims);

    for (uint32_t i = 0; i < prims_assembled; i++) {
        GLPrimitive* prim = &p_buf[i];
        if (prim->type == GL_POINTS && prim->vertex_count >= 1) {
            GLVertex v = prim->vertices[0];
            if ((v.clip_outcode & 0x3F) == 0) {
                GLScreenVertex sv;
                if (gl_viewport_transform_vertex(&v, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv)) {
                    gl_rasterize_point(&sv, state->point_size, &target, state);
                }
            }
        } else if (prim->type == GL_LINES && prim->vertex_count >= 2) {
            GLVertex v0 = prim->vertices[0];
            GLVertex v1 = prim->vertices[1];
            if ((v0.clip_outcode & v1.clip_outcode & 0x3F) == 0) {
                GLScreenVertex sv0, sv1;
                bool ok0 = gl_viewport_transform_vertex(&v0, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv0);
                bool ok1 = gl_viewport_transform_vertex(&v1, state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &sv1);
                if (ok0 && ok1) {
                    gl_rasterize_line(&sv0, &sv1, state->line_width, &target, state);
                }
            }
        } else if (prim->type == GL_TRIANGLES && prim->vertex_count >= 3) {
            if (state->shade_model == GL_FLAT) {
                prim->vertices[0].color[0] = prim->vertices[2].color[0];
                prim->vertices[0].color[1] = prim->vertices[2].color[1];
                prim->vertices[0].color[2] = prim->vertices[2].color[2];
                prim->vertices[0].color[3] = prim->vertices[2].color[3];
                prim->vertices[1].color[0] = prim->vertices[2].color[0];
                prim->vertices[1].color[1] = prim->vertices[2].color[1];
                prim->vertices[1].color[2] = prim->vertices[2].color[2];
                prim->vertices[1].color[3] = prim->vertices[2].color[3];
            }
            GLVertex clipped_poly[GL_MAX_CLIPPED_POLYGON_VERTICES];
            uint32_t poly_count = gl_clip_triangle(prim->vertices, clipped_poly);
            if (poly_count < 3) continue;
            GLPrimitive clipped_tris[GL_MAX_CLIPPED_POLYGON_VERTICES];
            uint32_t tri_count = gl_triangulate_polygon(clipped_poly, poly_count, clipped_tris, GL_MAX_CLIPPED_POLYGON_VERTICES);
            for (uint32_t t = 0; t < tri_count; t++) {
                GLScreenVertex screen_v[3];
                bool v0_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[0], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[0]);
                bool v1_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[1], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[1]);
                bool v2_ok = gl_viewport_transform_vertex(&clipped_tris[t].vertices[2], state->viewport_x, state->viewport_y, state->viewport_w, state->viewport_h, &screen_v[2]);
                if (v0_ok && v1_ok && v2_ok) {
                    gl_rasterize_triangle(screen_v, &target, state);
                }
            }
        }
    }

    if (free_v_buf) kfree(v_buf);
    if (free_p_buf) kfree(p_buf);
}

/* --- Phase 8 Pixel Storage, Sub-Images, Mipmapping & Readback --- */

void glPixelStorei(GLenum pname, GLint param) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_PIXEL_STOREI;
        cmd.data.pixel_store.pname = pname;
        cmd.data.pixel_store.param = param;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    switch (pname) {
        case GL_UNPACK_ALIGNMENT:
            if (param != 1 && param != 2 && param != 4 && param != 8) {
                gl_state_set_error(state, GL_INVALID_VALUE);
                return;
            }
            state->unpack_alignment = param;
            break;
        case GL_PACK_ALIGNMENT:
            if (param != 1 && param != 2 && param != 4 && param != 8) {
                gl_state_set_error(state, GL_INVALID_VALUE);
                return;
            }
            state->pack_alignment = param;
            break;
        default:
            gl_state_set_error(state, GL_INVALID_ENUM);
            break;
    }
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_TEX_SUB_IMAGE_2D;
        cmd.data.tex_sub_image_2d.level = level;
        cmd.data.tex_sub_image_2d.xoffset = xoffset;
        cmd.data.tex_sub_image_2d.yoffset = yoffset;
        cmd.data.tex_sub_image_2d.width = width;
        cmd.data.tex_sub_image_2d.height = height;
        cmd.data.tex_sub_image_2d.format = format;
        cmd.data.tex_sub_image_2d.type = type;
        size_t bpp = (format == GL_RGBA) ? 4 : 3;
        size_t unaligned_row = (size_t)width * bpp;
        size_t align = (size_t)state->unpack_alignment;
        size_t row_stride = (unaligned_row + align - 1) & ~(align - 1);
        size_t payload_sz = row_stride * (size_t)height;
        cmd.data.tex_sub_image_2d.payload_size = payload_sz;
        if (pixels && payload_sz > 0) {
            cmd.data.tex_sub_image_2d.pixel_payload = kmalloc(payload_sz);
            if (cmd.data.tex_sub_image_2d.pixel_payload) {
                const uint8_t* src_p = (const uint8_t*)pixels;
                uint8_t* dst_p = (uint8_t*)cmd.data.tex_sub_image_2d.pixel_payload;
                for (size_t i = 0; i < payload_sz; i++) dst_p[i] = src_p[i];
            }
        } else {
            cmd.data.tex_sub_image_2d.pixel_payload = NULL;
        }
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    GLTextureObject* tex = gl_state_get_bound_texture(state);
    if (!tex) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (!gl_texture_sub_upload_image(tex, level, xoffset, yoffset, width, height, format, type, pixels, state->unpack_alignment)) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }
}

void glGenerateMipmap(GLenum target) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_GENERATE_MIPMAP;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    GLTextureObject* tex = gl_state_get_bound_texture(state);
    if (!tex) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (!gl_texture_generate_mipmaps(tex)) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    state->mipmap_generations++;
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (format != GL_RGB && format != GL_RGBA) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (type != GL_UNSIGNED_BYTE) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (width < 0 || height < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    if (width == 0 || height == 0 || !pixels) return;

    GLRenderTarget target;
    if (!gl_get_active_render_target(state, &target) || !target.has_color || !target.color_buffer) {
        return;
    }

    size_t bpp = (format == GL_RGBA) ? 4 : 3;
    size_t unaligned_row = (size_t)width * bpp;
    size_t align = (size_t)state->pack_alignment;
    size_t pack_stride = (unaligned_row + align - 1) & ~(align - 1);

    for (GLsizei row = 0; row < height; row++) {
        GLint gl_y = y + row;
        GLint bgl_y = ((GLint)target.height - 1) - gl_y;
        uint8_t* dst_row = (uint8_t*)pixels + (size_t)row * pack_stride;

        for (GLsizei col = 0; col < width; col++) {
            GLint bgl_x = x + col;

            uint8_t r = 0, g = 0, b = 0, a = 255;
            if (bgl_x >= 0 && bgl_x < (GLint)target.width && bgl_y >= 0 && bgl_y < (GLint)target.height) {
                uint32_t argb = target.color_buffer[(size_t)bgl_y * (size_t)target.color_pitch + (size_t)bgl_x];
                a = (uint8_t)((argb >> 24) & 0xFF);
                r = (uint8_t)((argb >> 16) & 0xFF);
                g = (uint8_t)((argb >> 8) & 0xFF);
                b = (uint8_t)(argb & 0xFF);
            }

            if (format == GL_RGBA) {
                dst_row[col * 4 + 0] = r;
                dst_row[col * 4 + 1] = g;
                dst_row[col * 4 + 2] = b;
                dst_row[col * 4 + 3] = a;
            } else { // GL_RGB
                dst_row[col * 3 + 0] = r;
                dst_row[col * 3 + 1] = g;
                dst_row[col * 3 + 2] = b;
            }
        }
    }

    state->read_pixels_calls++;
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (border != 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    GLTextureObject* tex = gl_state_get_bound_texture(state);
    if (!tex) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (width <= 0 || height <= 0 || width > GL_MAX_TEXTURE_SIZE || height > GL_MAX_TEXTURE_SIZE) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    size_t buf_size = (size_t)width * (size_t)height * 4;
    uint8_t* tmp_buf = (uint8_t*)kmalloc_aligned(buf_size, 16);
    if (!tmp_buf) {
        gl_state_set_error(state, GL_OUT_OF_MEMORY);
        return;
    }

    // Temporary save pack_alignment
    GLint old_pack = state->pack_alignment;
    state->pack_alignment = 4;
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tmp_buf);
    state->pack_alignment = old_pack;

    bool ok = gl_texture_upload_image(tex, level, internalformat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_buf, 4);
    kfree_aligned(tmp_buf);

    if (!ok) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->copy_texture_calls++;
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_TEXTURE_2D) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLTextureObject* tex = gl_state_get_bound_texture(state);
    if (!tex) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (width < 0 || height < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    if (width == 0 || height == 0) return;

    size_t buf_size = (size_t)width * (size_t)height * 4;
    uint8_t* tmp_buf = (uint8_t*)kmalloc_aligned(buf_size, 16);
    if (!tmp_buf) {
        gl_state_set_error(state, GL_OUT_OF_MEMORY);
        return;
    }

    GLint old_pack = state->pack_alignment;
    state->pack_alignment = 4;
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tmp_buf);
    state->pack_alignment = old_pack;

    bool ok = gl_texture_sub_upload_image(tex, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tmp_buf, 4);
    kfree_aligned(tmp_buf);

    if (!ok) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    state->copy_texture_calls++;
}

/* --- Phase 9 Stencil, Polygon Offset & Raster State APIs --- */
static bool is_valid_stencil_op(GLenum op) {
    return (op == GL_KEEP || op == GL_ZERO || op == GL_REPLACE ||
            op == GL_INCR || op == GL_DECR || op == GL_INVERT ||
            op == GL_INCR_WRAP || op == GL_DECR_WRAP);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (func < GL_NEVER || func > GL_ALWAYS) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_STENCIL_FUNC;
        cmd.data.stencil_func.func = func;
        cmd.data.stencil_func.ref = ref;
        cmd.data.stencil_func.mask = mask;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->stencil_func = func;
    state->stencil_ref = ref;
    state->stencil_value_mask = mask;
}

void glStencilMask(GLuint mask) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_STENCIL_MASK;
        cmd.data.stencil_mask.mask = mask;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->stencil_write_mask = mask;
}

void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (!is_valid_stencil_op(sfail) || !is_valid_stencil_op(dpfail) || !is_valid_stencil_op(dppass)) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_STENCIL_OP;
        cmd.data.stencil_op.sfail = sfail;
        cmd.data.stencil_op.dpfail = dpfail;
        cmd.data.stencil_op.dppass = dppass;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->stencil_fail_op = sfail;
    state->stencil_depth_fail_op = dpfail;
    state->stencil_depth_pass_op = dppass;
}

void glClearStencil(GLint s) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_CLEAR_STENCIL;
        cmd.data.clear_stencil.s = s;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->clear_stencil = s;
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_POLYGON_OFFSET;
        cmd.data.polygon_offset.factor = factor;
        cmd.data.polygon_offset.units = units;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    state->polygon_offset_factor = factor;
    state->polygon_offset_units = units;
}

/* --- Phase 10 Public FBO / RBO APIs --- */

void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
    GLContextState* state = gl_state_get_current();
    if (!state || !framebuffers) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        framebuffers[i] = gl_fbo_gen_framebuffer(state);
    }
}

void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    GLContextState* state = gl_state_get_current();
    if (!state || !framebuffers) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        gl_fbo_delete_framebuffer(state, framebuffers[i]);
    }
}

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (framebuffer != 0) {
        GLFramebufferObject* fbo = gl_fbo_get_framebuffer(state, framebuffer);
        if (!fbo) {
            // Auto-allocate if within bounds
            if (framebuffer < GL_MAX_FRAMEBUFFERS) {
                state->framebuffers[framebuffer].id = framebuffer;
                state->framebuffers[framebuffer].allocated = true;
                fbo = &state->framebuffers[framebuffer];
            } else {
                gl_state_set_error(state, GL_INVALID_VALUE);
                return;
            }
        }
    }

    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
        state->bound_draw_framebuffer = framebuffer;
    }
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
        state->bound_read_framebuffer = framebuffer;
    }

    state->framebuffer_binds++;
    state->render_target_switches++;
}

GLboolean glIsFramebuffer(GLuint framebuffer) {
    GLContextState* state = gl_state_get_current();
    if (!state || framebuffer == 0) return GL_FALSE;

    GLFramebufferObject* fbo = gl_fbo_get_framebuffer(state, framebuffer);
    return (fbo && fbo->allocated) ? GL_TRUE : GL_FALSE;
}

GLenum glCheckFramebufferStatus(GLenum target) {
    GLContextState* state = gl_state_get_current();
    if (!state) return 0;

    if (target != GL_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return 0;
    }

    GLuint fbo_id = (target == GL_READ_FRAMEBUFFER) ? state->bound_read_framebuffer : state->bound_draw_framebuffer;
    return gl_fbo_check_completeness(state, fbo_id);
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLuint fbo_id = (target == GL_READ_FRAMEBUFFER) ? state->bound_read_framebuffer : state->bound_draw_framebuffer;
    if (fbo_id == 0) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLFramebufferObject* fbo = gl_fbo_get_framebuffer(state, fbo_id);
    if (!fbo) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (textarget != GL_TEXTURE_2D && texture != 0) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLFramebufferAttachment* att_entry = NULL;
    if (attachment == GL_COLOR_ATTACHMENT0) {
        att_entry = &fbo->color_attachment0;
    } else if (attachment == GL_DEPTH_ATTACHMENT) {
        att_entry = &fbo->depth_attachment;
    } else if (attachment == GL_STENCIL_ATTACHMENT) {
        att_entry = &fbo->stencil_attachment;
    } else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        fbo->depth_attachment.type = (texture == 0) ? GL_ATTACHMENT_NONE : GL_ATTACHMENT_TEXTURE_2D;
        fbo->depth_attachment.object_id = texture;
        fbo->depth_attachment.texture_level = level;
        fbo->stencil_attachment.type = (texture == 0) ? GL_ATTACHMENT_NONE : GL_ATTACHMENT_TEXTURE_2D;
        fbo->stencil_attachment.object_id = texture;
        fbo->stencil_attachment.texture_level = level;
        return;
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (texture == 0) {
        att_entry->type = GL_ATTACHMENT_NONE;
        att_entry->object_id = 0;
        att_entry->texture_level = 0;
    } else {
        att_entry->type = GL_ATTACHMENT_TEXTURE_2D;
        att_entry->object_id = texture;
        att_entry->texture_level = level;
    }
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER && target != GL_READ_FRAMEBUFFER) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLuint fbo_id = (target == GL_READ_FRAMEBUFFER) ? state->bound_read_framebuffer : state->bound_draw_framebuffer;
    if (fbo_id == 0) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLFramebufferObject* fbo = gl_fbo_get_framebuffer(state, fbo_id);
    if (!fbo) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (renderbuffertarget != GL_RENDERBUFFER && renderbuffer != 0) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLFramebufferAttachment* att_entry = NULL;
    if (attachment == GL_COLOR_ATTACHMENT0) {
        att_entry = &fbo->color_attachment0;
    } else if (attachment == GL_DEPTH_ATTACHMENT) {
        att_entry = &fbo->depth_attachment;
    } else if (attachment == GL_STENCIL_ATTACHMENT) {
        att_entry = &fbo->stencil_attachment;
    } else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        fbo->depth_attachment.type = (renderbuffer == 0) ? GL_ATTACHMENT_NONE : GL_ATTACHMENT_RENDERBUFFER;
        fbo->depth_attachment.object_id = renderbuffer;
        fbo->depth_attachment.texture_level = 0;
        fbo->stencil_attachment.type = (renderbuffer == 0) ? GL_ATTACHMENT_NONE : GL_ATTACHMENT_RENDERBUFFER;
        fbo->stencil_attachment.object_id = renderbuffer;
        fbo->stencil_attachment.texture_level = 0;
        return;
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (renderbuffer == 0) {
        att_entry->type = GL_ATTACHMENT_NONE;
        att_entry->object_id = 0;
        att_entry->texture_level = 0;
    } else {
        att_entry->type = GL_ATTACHMENT_RENDERBUFFER;
        att_entry->object_id = renderbuffer;
        att_entry->texture_level = 0;
    }
}

void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GLContextState* state = gl_state_get_current();
    if (!state || !renderbuffers) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        renderbuffers[i] = gl_rbo_gen_renderbuffer(state);
    }
}

void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    GLContextState* state = gl_state_get_current();
    if (!state || !renderbuffers) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (n < 0) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        gl_rbo_delete_renderbuffer(state, renderbuffers[i]);
    }
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_RENDERBUFFER) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (renderbuffer != 0) {
        GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, renderbuffer);
        if (!rbo) {
            if (renderbuffer < GL_MAX_RENDERBUFFERS) {
                state->renderbuffers[renderbuffer].id = renderbuffer;
                state->renderbuffers[renderbuffer].allocated = true;
            } else {
                gl_state_set_error(state, GL_INVALID_VALUE);
                return;
            }
        }
    }

    state->bound_renderbuffer = renderbuffer;
}

GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    GLContextState* state = gl_state_get_current();
    if (!state || renderbuffer == 0) return GL_FALSE;

    GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, renderbuffer);
    return (rbo && rbo->allocated) ? GL_TRUE : GL_FALSE;
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (target != GL_RENDERBUFFER) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->bound_renderbuffer == 0) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    if (width <= 0 || height <= 0 || width > GL_MAX_RENDERBUFFER_SIZE || height > GL_MAX_RENDERBUFFER_SIZE) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    size_t elem_size = 0;
    if (internalformat == GL_RGBA8 || internalformat == GL_RGBA4 || internalformat == GL_RGB5_A1) {
        elem_size = sizeof(uint32_t); // 4 bytes/pixel
    } else if (internalformat == GL_DEPTH_COMPONENT16 || internalformat == GL_DEPTH_COMPONENT24) {
        elem_size = sizeof(float); // 4 bytes/pixel
    } else if (internalformat == GL_STENCIL_INDEX8) {
        elem_size = sizeof(uint8_t); // 1 byte/pixel
    } else if (internalformat == GL_DEPTH24_STENCIL8) {
        elem_size = sizeof(float) + sizeof(uint8_t); // packed
    } else {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, state->bound_renderbuffer);
    if (!rbo) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    size_t new_size = (size_t)width * (size_t)height * elem_size;
    if (rbo->storage_buffer) {
        kfree_aligned(rbo->storage_buffer);
        rbo->storage_buffer = NULL;
    }

    void* new_buf = kmalloc_aligned(new_size, 16);
    if (!new_buf) {
        gl_state_set_error(state, GL_OUT_OF_MEMORY);
        return;
    }

    // Zero-initialize buffer
    uint8_t* p = (uint8_t*)new_buf;
    for (size_t i = 0; i < new_size; i++) p[i] = 0;

    rbo->internal_format = internalformat;
    rbo->width = width;
    rbo->height = height;
    rbo->storage_buffer = new_buf;
    rbo->storage_size = new_size;
}
