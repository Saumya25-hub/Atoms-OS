#include "kernel/graphics/gl/gl_fbo.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/core/memory/heap/include/heap.h"

void gl_fbo_system_init(GLContextState* state) {
    if (!state) return;

    state->bound_draw_framebuffer = 0;
    state->bound_read_framebuffer = 0;
    state->bound_renderbuffer = 0;

    for (int i = 0; i < GL_MAX_FRAMEBUFFERS; i++) {
        state->framebuffers[i].id = 0;
        state->framebuffers[i].allocated = false;
        state->framebuffers[i].color_attachment0.type = GL_ATTACHMENT_NONE;
        state->framebuffers[i].color_attachment0.object_id = 0;
        state->framebuffers[i].color_attachment0.texture_level = 0;
        state->framebuffers[i].depth_attachment.type = GL_ATTACHMENT_NONE;
        state->framebuffers[i].depth_attachment.object_id = 0;
        state->framebuffers[i].depth_attachment.texture_level = 0;
        state->framebuffers[i].stencil_attachment.type = GL_ATTACHMENT_NONE;
        state->framebuffers[i].stencil_attachment.object_id = 0;
        state->framebuffers[i].stencil_attachment.texture_level = 0;
    }

    for (int i = 0; i < GL_MAX_RENDERBUFFERS; i++) {
        state->renderbuffers[i].id = 0;
        state->renderbuffers[i].allocated = false;
        state->renderbuffers[i].internal_format = 0;
        state->renderbuffers[i].width = 0;
        state->renderbuffers[i].height = 0;
        state->renderbuffers[i].storage_buffer = NULL;
        state->renderbuffers[i].storage_size = 0;
    }
}

void gl_fbo_system_cleanup(GLContextState* state) {
    if (!state) return;

    for (int i = 0; i < GL_MAX_RENDERBUFFERS; i++) {
        if (state->renderbuffers[i].storage_buffer) {
            kfree_aligned(state->renderbuffers[i].storage_buffer);
            state->renderbuffers[i].storage_buffer = NULL;
        }
        state->renderbuffers[i].allocated = false;
    }

    for (int i = 0; i < GL_MAX_FRAMEBUFFERS; i++) {
        state->framebuffers[i].allocated = false;
    }
}

GLuint gl_fbo_gen_framebuffer(GLContextState* state) {
    if (!state) return 0;

    for (GLuint i = 1; i < GL_MAX_FRAMEBUFFERS; i++) {
        if (!state->framebuffers[i].allocated) {
            state->framebuffers[i].id = i;
            state->framebuffers[i].allocated = true;
            state->framebuffers[i].color_attachment0.type = GL_ATTACHMENT_NONE;
            state->framebuffers[i].color_attachment0.object_id = 0;
            state->framebuffers[i].color_attachment0.texture_level = 0;
            state->framebuffers[i].depth_attachment.type = GL_ATTACHMENT_NONE;
            state->framebuffers[i].depth_attachment.object_id = 0;
            state->framebuffers[i].depth_attachment.texture_level = 0;
            state->framebuffers[i].stencil_attachment.type = GL_ATTACHMENT_NONE;
            state->framebuffers[i].stencil_attachment.object_id = 0;
            state->framebuffers[i].stencil_attachment.texture_level = 0;
            return i;
        }
    }
    return 0;
}

bool gl_fbo_delete_framebuffer(GLContextState* state, GLuint id) {
    if (!state || id == 0 || id >= GL_MAX_FRAMEBUFFERS) return false;

    GLFramebufferObject* fbo = &state->framebuffers[id];
    if (!fbo->allocated) return false;

    fbo->allocated = false;
    fbo->color_attachment0.type = GL_ATTACHMENT_NONE;
    fbo->depth_attachment.type = GL_ATTACHMENT_NONE;
    fbo->stencil_attachment.type = GL_ATTACHMENT_NONE;

    if (state->bound_draw_framebuffer == id) state->bound_draw_framebuffer = 0;
    if (state->bound_read_framebuffer == id) state->bound_read_framebuffer = 0;

    return true;
}

GLFramebufferObject* gl_fbo_get_framebuffer(GLContextState* state, GLuint id) {
    if (!state || id >= GL_MAX_FRAMEBUFFERS) return NULL;

    GLFramebufferObject* fbo = &state->framebuffers[id];
    if (!fbo->allocated && id != 0) return NULL;
    return fbo;
}

GLuint gl_rbo_gen_renderbuffer(GLContextState* state) {
    if (!state) return 0;

    for (GLuint i = 1; i < GL_MAX_RENDERBUFFERS; i++) {
        if (!state->renderbuffers[i].allocated) {
            state->renderbuffers[i].id = i;
            state->renderbuffers[i].allocated = true;
            state->renderbuffers[i].internal_format = 0;
            state->renderbuffers[i].width = 0;
            state->renderbuffers[i].height = 0;
            state->renderbuffers[i].storage_buffer = NULL;
            state->renderbuffers[i].storage_size = 0;
            return i;
        }
    }
    return 0;
}

bool gl_rbo_delete_renderbuffer(GLContextState* state, GLuint id) {
    if (!state || id == 0 || id >= GL_MAX_RENDERBUFFERS) return false;

    GLRenderbufferObject* rbo = &state->renderbuffers[id];
    if (!rbo->allocated) return false;

    if (rbo->storage_buffer) {
        kfree_aligned(rbo->storage_buffer);
        rbo->storage_buffer = NULL;
    }

    rbo->allocated = false;
    rbo->width = 0;
    rbo->height = 0;
    rbo->internal_format = 0;
    rbo->storage_size = 0;

    if (state->bound_renderbuffer == id) state->bound_renderbuffer = 0;

    gl_fbo_on_renderbuffer_deleted(state, id);
    return true;
}

GLRenderbufferObject* gl_rbo_get_renderbuffer(GLContextState* state, GLuint id) {
    if (!state || id >= GL_MAX_RENDERBUFFERS) return NULL;

    GLRenderbufferObject* rbo = &state->renderbuffers[id];
    if (!rbo->allocated) return NULL;
    return rbo;
}

void gl_fbo_on_texture_deleted(GLContextState* state, GLuint texture_id) {
    if (!state || texture_id == 0) return;

    for (int i = 1; i < GL_MAX_FRAMEBUFFERS; i++) {
        if (!state->framebuffers[i].allocated) continue;
        GLFramebufferObject* fbo = &state->framebuffers[i];
        if (fbo->color_attachment0.type == GL_ATTACHMENT_TEXTURE_2D && fbo->color_attachment0.object_id == texture_id) {
            fbo->color_attachment0.type = GL_ATTACHMENT_NONE;
            fbo->color_attachment0.object_id = 0;
        }
        if (fbo->depth_attachment.type == GL_ATTACHMENT_TEXTURE_2D && fbo->depth_attachment.object_id == texture_id) {
            fbo->depth_attachment.type = GL_ATTACHMENT_NONE;
            fbo->depth_attachment.object_id = 0;
        }
        if (fbo->stencil_attachment.type == GL_ATTACHMENT_TEXTURE_2D && fbo->stencil_attachment.object_id == texture_id) {
            fbo->stencil_attachment.type = GL_ATTACHMENT_NONE;
            fbo->stencil_attachment.object_id = 0;
        }
    }
}

void gl_fbo_on_renderbuffer_deleted(GLContextState* state, GLuint rbo_id) {
    if (!state || rbo_id == 0) return;

    for (int i = 1; i < GL_MAX_FRAMEBUFFERS; i++) {
        if (!state->framebuffers[i].allocated) continue;
        GLFramebufferObject* fbo = &state->framebuffers[i];
        if (fbo->color_attachment0.type == GL_ATTACHMENT_RENDERBUFFER && fbo->color_attachment0.object_id == rbo_id) {
            fbo->color_attachment0.type = GL_ATTACHMENT_NONE;
            fbo->color_attachment0.object_id = 0;
        }
        if (fbo->depth_attachment.type == GL_ATTACHMENT_RENDERBUFFER && fbo->depth_attachment.object_id == rbo_id) {
            fbo->depth_attachment.type = GL_ATTACHMENT_NONE;
            fbo->depth_attachment.object_id = 0;
        }
        if (fbo->stencil_attachment.type == GL_ATTACHMENT_RENDERBUFFER && fbo->stencil_attachment.object_id == rbo_id) {
            fbo->stencil_attachment.type = GL_ATTACHMENT_NONE;
            fbo->stencil_attachment.object_id = 0;
        }
    }
}

GLenum gl_fbo_check_completeness(GLContextState* state, GLuint fbo_id) {
    if (!state) return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    state->framebuffer_completeness_checks++;

    if (fbo_id == 0) return GL_FRAMEBUFFER_COMPLETE;

    if (fbo_id >= GL_MAX_FRAMEBUFFERS) {
        state->framebuffer_incomplete_attempts++;
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }

    GLFramebufferObject* fbo = &state->framebuffers[fbo_id];
    if (!fbo->allocated) {
        state->framebuffer_incomplete_attempts++;
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }

    bool has_color = (fbo->color_attachment0.type != GL_ATTACHMENT_NONE);
    bool has_depth = (fbo->depth_attachment.type != GL_ATTACHMENT_NONE);
    bool has_stencil = (fbo->stencil_attachment.type != GL_ATTACHMENT_NONE);

    if (!has_color && !has_depth && !has_stencil) {
        state->framebuffer_incomplete_attempts++;
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }

    GLsizei w = 0, h = 0;

    // Check Color Attachment
    if (has_color) {
        if (fbo->color_attachment0.type == GL_ATTACHMENT_TEXTURE_2D) {
            GLTextureObject* tex = gl_state_get_texture(state, fbo->color_attachment0.object_id);
            if (!tex || !tex->allocated || !tex->levels[0].pixel_data) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            w = tex->levels[0].width;
            h = tex->levels[0].height;
        } else if (fbo->color_attachment0.type == GL_ATTACHMENT_RENDERBUFFER) {
            GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, fbo->color_attachment0.object_id);
            if (!rbo || !rbo->allocated || !rbo->storage_buffer || rbo->internal_format != GL_RGBA8) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            w = rbo->width;
            h = rbo->height;
        } else {
            state->framebuffer_incomplete_attempts++;
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }
    }

    // Check Depth Attachment
    if (has_depth) {
        GLsizei dw = 0, dh = 0;
        if (fbo->depth_attachment.type == GL_ATTACHMENT_RENDERBUFFER) {
            GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, fbo->depth_attachment.object_id);
            if (!rbo || !rbo->allocated || !rbo->storage_buffer) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            if (rbo->internal_format != GL_DEPTH_COMPONENT16 && 
                rbo->internal_format != GL_DEPTH_COMPONENT24 && 
                rbo->internal_format != GL_DEPTH24_STENCIL8) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            dw = rbo->width;
            dh = rbo->height;
        } else if (fbo->depth_attachment.type == GL_ATTACHMENT_TEXTURE_2D) {
            GLTextureObject* tex = gl_state_get_texture(state, fbo->depth_attachment.object_id);
            if (!tex || !tex->allocated || !tex->levels[0].pixel_data) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            dw = tex->levels[0].width;
            dh = tex->levels[0].height;
        } else {
            state->framebuffer_incomplete_attempts++;
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (w == 0 && h == 0) {
            w = dw;
            h = dh;
        } else if (w != dw || h != dh) {
            state->framebuffer_incomplete_attempts++;
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
    }

    // Check Stencil Attachment
    if (has_stencil) {
        GLsizei sw = 0, sh = 0;
        if (fbo->stencil_attachment.type == GL_ATTACHMENT_RENDERBUFFER) {
            GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, fbo->stencil_attachment.object_id);
            if (!rbo || !rbo->allocated || !rbo->storage_buffer) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            if (rbo->internal_format != GL_STENCIL_INDEX8 && 
                rbo->internal_format != GL_DEPTH24_STENCIL8) {
                state->framebuffer_incomplete_attempts++;
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            sw = rbo->width;
            sh = rbo->height;
        } else {
            state->framebuffer_incomplete_attempts++;
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        if (w == 0 && h == 0) {
            w = sw;
            h = sh;
        } else if (w != sw || h != sh) {
            state->framebuffer_incomplete_attempts++;
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
    }

    if (w <= 0 || h <= 0) {
        state->framebuffer_incomplete_attempts++;
        return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
    }

    return GL_FRAMEBUFFER_COMPLETE;
}

bool gl_get_active_render_target(GLContextState* state, GLRenderTarget* out_target) {
    if (!state || !out_target) return false;

    out_target->color_buffer = NULL;
    out_target->depth_buffer = NULL;
    out_target->stencil_buffer = NULL;
    out_target->width = 0;
    out_target->height = 0;
    out_target->color_pitch = 0;
    out_target->depth_pitch = 0;
    out_target->stencil_pitch = 0;
    out_target->is_fbo = false;
    out_target->has_color = false;
    out_target->has_depth = false;
    out_target->has_stencil = false;
    out_target->fbo_id = state->bound_draw_framebuffer;
    out_target->color_tex_obj = NULL;

    // Framebuffer 0: Default Window BGLDrawable
    if (state->bound_draw_framebuffer == 0) {
        BGLContext* bgl_ctx = bglGetCurrentContext();
        if (!bgl_ctx || !bgl_ctx->bound_drawable || !bgl_ctx->bound_drawable->active) return false;

        BGLDrawable* d = bgl_ctx->bound_drawable;
        out_target->color_buffer = d->color_buffer;
        out_target->depth_buffer = d->depth_buffer;
        out_target->stencil_buffer = d->stencil_buffer;
        out_target->width = d->width;
        out_target->height = d->height;
        out_target->color_pitch = d->pitch / sizeof(uint32_t);
        out_target->depth_pitch = d->width;
        out_target->stencil_pitch = d->width;
        out_target->is_fbo = false;
        out_target->has_color = (d->color_buffer != NULL);
        out_target->has_depth = (d->depth_buffer != NULL);
        out_target->has_stencil = (d->stencil_buffer != NULL);
        return true;
    }

    // User Framebuffer Object (FBO > 0)
    GLenum status = gl_fbo_check_completeness(state, state->bound_draw_framebuffer);
    if (status != GL_FRAMEBUFFER_COMPLETE) return false;

    GLFramebufferObject* fbo = gl_fbo_get_framebuffer(state, state->bound_draw_framebuffer);
    if (!fbo) return false;

    out_target->is_fbo = true;

    // Resolve Color Attachment 0
    if (fbo->color_attachment0.type == GL_ATTACHMENT_TEXTURE_2D) {
        GLTextureObject* tex = gl_state_get_texture(state, fbo->color_attachment0.object_id);
        if (tex && tex->allocated && tex->levels[0].pixel_data) {
            out_target->color_buffer = (uint32_t*)tex->levels[0].pixel_data;
            out_target->width = tex->levels[0].width;
            out_target->height = tex->levels[0].height;
            out_target->color_pitch = tex->levels[0].width;
            out_target->has_color = true;
            out_target->color_tex_obj = tex;
        }
    } else if (fbo->color_attachment0.type == GL_ATTACHMENT_RENDERBUFFER) {
        GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, fbo->color_attachment0.object_id);
        if (rbo && rbo->allocated && rbo->storage_buffer) {
            out_target->color_buffer = (uint32_t*)rbo->storage_buffer;
            out_target->width = rbo->width;
            out_target->height = rbo->height;
            out_target->color_pitch = rbo->width;
            out_target->has_color = true;
        }
    }

    // Resolve Depth Attachment
    if (fbo->depth_attachment.type == GL_ATTACHMENT_RENDERBUFFER) {
        GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, fbo->depth_attachment.object_id);
        if (rbo && rbo->allocated && rbo->storage_buffer) {
            out_target->depth_buffer = (float*)rbo->storage_buffer;
            out_target->depth_pitch = rbo->width;
            out_target->has_depth = true;
            if (out_target->width == 0) {
                out_target->width = rbo->width;
                out_target->height = rbo->height;
            }
        }
    } else if (fbo->depth_attachment.type == GL_ATTACHMENT_TEXTURE_2D) {
        GLTextureObject* tex = gl_state_get_texture(state, fbo->depth_attachment.object_id);
        if (tex && tex->allocated && tex->levels[0].pixel_data) {
            out_target->depth_buffer = (float*)tex->levels[0].pixel_data;
            out_target->depth_pitch = tex->levels[0].width;
            out_target->has_depth = true;
            if (out_target->width == 0) {
                out_target->width = tex->levels[0].width;
                out_target->height = tex->levels[0].height;
            }
        }
    }

    // Resolve Stencil Attachment
    if (fbo->stencil_attachment.type == GL_ATTACHMENT_RENDERBUFFER) {
        GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, fbo->stencil_attachment.object_id);
        if (rbo && rbo->allocated && rbo->storage_buffer) {
            out_target->stencil_buffer = (uint8_t*)rbo->storage_buffer;
            out_target->stencil_pitch = rbo->width;
            out_target->has_stencil = true;
            if (out_target->width == 0) {
                out_target->width = rbo->width;
                out_target->height = rbo->height;
            }
        }
    }

    return (out_target->width > 0 && out_target->height > 0);
}

bool gl_fbo_is_sampler_aliased(GLContextState* state, const GLTextureObject* sampler_tex) {
    if (!state || !sampler_tex || state->bound_draw_framebuffer == 0) return false;

    GLRenderTarget target;
    if (gl_get_active_render_target(state, &target)) {
        if (target.is_fbo && target.color_tex_obj == sampler_tex) {
            state->feedback_loop_hazards++;
            return true; // Hazard detected: texture sampler matches active FBO color attachment!
        }
    }
    return false;
}
