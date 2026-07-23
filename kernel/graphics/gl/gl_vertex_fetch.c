#include "kernel/graphics/gl/gl_vertex_fetch.h"
#include "kernel/graphics/gl/gl_lighting.h"

uint32_t gl_client_array_get_effective_stride(GLint size, GLenum type, GLsizei stride) {
    if (stride > 0) return (uint32_t)stride;

    uint32_t type_size = sizeof(float);
    switch (type) {
        case GL_BYTE:           type_size = sizeof(int8_t); break;
        case GL_UNSIGNED_BYTE:  type_size = sizeof(uint8_t); break;
        case GL_SHORT:          type_size = sizeof(int16_t); break;
        case GL_UNSIGNED_SHORT: type_size = sizeof(uint16_t); break;
        case GL_INT:            type_size = sizeof(int32_t); break;
        case GL_UNSIGNED_INT:   type_size = sizeof(uint32_t); break;
        case GL_FLOAT:          type_size = sizeof(float); break;
        case GL_DOUBLE:         type_size = sizeof(double); break;
        default:                type_size = sizeof(float); break;
    }
    return (uint32_t)size * type_size;
}

static float read_element_as_float(const void* ptr, GLenum type, bool normalize_ubyte) {
    if (!ptr) return 0.0f;
    switch (type) {
        case GL_BYTE:           return (float)(*(const int8_t*)ptr);
        case GL_UNSIGNED_BYTE:  return normalize_ubyte ? ((float)(*(const uint8_t*)ptr) / 255.0f) : (float)(*(const uint8_t*)ptr);
        case GL_SHORT:          return normalize_ubyte ? ((float)(*(const int16_t*)ptr) / 32767.0f) : (float)(*(const int16_t*)ptr);
        case GL_UNSIGNED_SHORT: return normalize_ubyte ? ((float)(*(const uint16_t*)ptr) / 65535.0f) : (float)(*(const uint16_t*)ptr);
        case GL_INT:            return (float)(*(const int32_t*)ptr);
        case GL_UNSIGNED_INT:   return (float)(*(const uint32_t*)ptr);
        case GL_FLOAT:          return *(const float*)ptr;
        case GL_DOUBLE:         return (float)(*(const double*)ptr);
        default:                return 0.0f;
    }
}

static uint32_t get_type_size(GLenum type) {
    switch (type) {
        case GL_BYTE:           return sizeof(int8_t);
        case GL_UNSIGNED_BYTE:  return sizeof(uint8_t);
        case GL_SHORT:          return sizeof(int16_t);
        case GL_UNSIGNED_SHORT: return sizeof(uint16_t);
        case GL_INT:            return sizeof(int32_t);
        case GL_UNSIGNED_INT:   return sizeof(uint32_t);
        case GL_FLOAT:          return sizeof(float);
        case GL_DOUBLE:         return sizeof(double);
        default:                return sizeof(float);
    }
}

bool gl_fetch_client_vertex(GLContextState* state, uint32_t index, GLVertex* out_v) {
    if (!state || !out_v) return false;

    // 1. Fetch Position (GL_VERTEX_ARRAY must be enabled)
    if (!state->vertex_array.enabled || !state->vertex_array.pointer) {
        return false;
    }

    uint32_t v_stride = gl_client_array_get_effective_stride(state->vertex_array.size, state->vertex_array.type, state->vertex_array.stride);
    uint32_t v_elem_size = get_type_size(state->vertex_array.type);
    const uint8_t* v_ptr = (const uint8_t*)state->vertex_array.pointer + (index * v_stride);

    out_v->obj_pos.x = read_element_as_float(v_ptr, state->vertex_array.type, false);
    out_v->obj_pos.y = (state->vertex_array.size >= 2) ? read_element_as_float(v_ptr + v_elem_size, state->vertex_array.type, false) : 0.0f;
    out_v->obj_pos.z = (state->vertex_array.size >= 3) ? read_element_as_float(v_ptr + 2 * v_elem_size, state->vertex_array.type, false) : 0.0f;
    out_v->obj_pos.w = (state->vertex_array.size >= 4) ? read_element_as_float(v_ptr + 3 * v_elem_size, state->vertex_array.type, false) : 1.0f;

    // 2. Fetch Color (or default to current_color)
    if (state->color_array.enabled && state->color_array.pointer) {
        uint32_t c_stride = gl_client_array_get_effective_stride(state->color_array.size, state->color_array.type, state->color_array.stride);
        uint32_t c_elem_size = get_type_size(state->color_array.type);
        const uint8_t* c_ptr = (const uint8_t*)state->color_array.pointer + (index * c_stride);

        out_v->color[0] = read_element_as_float(c_ptr, state->color_array.type, true);
        out_v->color[1] = read_element_as_float(c_ptr + c_elem_size, state->color_array.type, true);
        out_v->color[2] = read_element_as_float(c_ptr + 2 * c_elem_size, state->color_array.type, true);
        out_v->color[3] = (state->color_array.size >= 4) ? read_element_as_float(c_ptr + 3 * c_elem_size, state->color_array.type, true) : 1.0f;
    } else {
        out_v->color[0] = state->current_color[0];
        out_v->color[1] = state->current_color[1];
        out_v->color[2] = state->current_color[2];
        out_v->color[3] = state->current_color[3];
    }

    // 3. Fetch Normal (or default to current_normal)
    if (state->normal_array.enabled && state->normal_array.pointer) {
        uint32_t n_stride = gl_client_array_get_effective_stride(3, state->normal_array.type, state->normal_array.stride);
        uint32_t n_elem_size = get_type_size(state->normal_array.type);
        const uint8_t* n_ptr = (const uint8_t*)state->normal_array.pointer + (index * n_stride);

        out_v->normal[0] = read_element_as_float(n_ptr, state->normal_array.type, false);
        out_v->normal[1] = read_element_as_float(n_ptr + n_elem_size, state->normal_array.type, false);
        out_v->normal[2] = read_element_as_float(n_ptr + 2 * n_elem_size, state->normal_array.type, false);
    } else {
        out_v->normal[0] = state->current_normal[0];
        out_v->normal[1] = state->current_normal[1];
        out_v->normal[2] = state->current_normal[2];
    }

    // 4. Fetch Texture Coords (or default to current_texcoord)
    if (state->texcoord_array.enabled && state->texcoord_array.pointer) {
        uint32_t t_stride = gl_client_array_get_effective_stride(state->texcoord_array.size, state->texcoord_array.type, state->texcoord_array.stride);
        uint32_t t_elem_size = get_type_size(state->texcoord_array.type);
        const uint8_t* t_ptr = (const uint8_t*)state->texcoord_array.pointer + (index * t_stride);

        out_v->texcoord[0] = read_element_as_float(t_ptr, state->texcoord_array.type, false);
        out_v->texcoord[1] = (state->texcoord_array.size >= 2) ? read_element_as_float(t_ptr + t_elem_size, state->texcoord_array.type, false) : 0.0f;
        out_v->texcoord[2] = (state->texcoord_array.size >= 3) ? read_element_as_float(t_ptr + 2 * t_elem_size, state->texcoord_array.type, false) : 0.0f;
        out_v->texcoord[3] = (state->texcoord_array.size >= 4) ? read_element_as_float(t_ptr + 3 * t_elem_size, state->texcoord_array.type, false) : 1.0f;
    } else {
        out_v->texcoord[0] = state->current_texcoord[0];
        out_v->texcoord[1] = state->current_texcoord[1];
        out_v->texcoord[2] = state->current_texcoord[2];
        out_v->texcoord[3] = state->current_texcoord[3];
    }

    // Process transformed vertex (ModelView * Position, Normal Matrix * Normal, Lighting, Clip Outcode)
    GLMatrix4x4* modelview = &state->modelview_stack[state->modelview_top];
    GLMatrix4x4* projection = &state->projection_stack[state->projection_top];
    gl_pipeline_transform_vertex(out_v, modelview, projection);

    return true;
}
