#include "kernel/graphics/gl/gl_depth.h"

bool gl_depth_test_pixel(GLenum depth_func, bool writemask, float frag_z, float* depth_ptr) {
    if (!depth_ptr) return true; // If no depth buffer, pass by default

    float buf_z = *depth_ptr;
    bool passed = false;

    switch (depth_func) {
        case GL_NEVER:    passed = false; break;
        case GL_LESS:     passed = (frag_z < buf_z); break;
        case GL_EQUAL:    passed = (frag_z == buf_z); break;
        case GL_LEQUAL:   passed = (frag_z <= buf_z); break;
        case GL_GREATER:  passed = (frag_z > buf_z); break;
        case GL_NOTEQUAL: passed = (frag_z != buf_z); break;
        case GL_GEQUAL:   passed = (frag_z >= buf_z); break;
        case GL_ALWAYS:   passed = true; break;
        default:          passed = (frag_z < buf_z); break;
    }

    if (passed && writemask) {
        *depth_ptr = frag_z;
    }

    return passed;
}

void gl_depth_clear(const GLRenderTarget* target, float clear_depth_val) {
    if (!target || !target->has_depth || !target->depth_buffer) return;

    size_t pixel_count = (size_t)target->width * (size_t)target->height;
    float* depth_buf = target->depth_buffer;

    for (size_t i = 0; i < pixel_count; i++) {
        depth_buf[i] = clear_depth_val;
    }
}
