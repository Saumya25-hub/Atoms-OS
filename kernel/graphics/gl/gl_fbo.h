#ifndef ATOMS_OS_GL_FBO_H
#define ATOMS_OS_GL_FBO_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_texture.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Unified Active Render Target Abstraction */
typedef struct {
    uint32_t* color_buffer;    // ARGB8888 buffer pointer
    float*    depth_buffer;    // 32-bit float depth buffer pointer
    uint8_t*  stencil_buffer;  // 8-bit stencil buffer pointer
    uint32_t  width;           // target width in pixels
    uint32_t  height;          // target height in pixels
    uint32_t  color_pitch;     // pitch in uint32_t elements per row
    uint32_t  depth_pitch;     // pitch in float elements per row
    uint32_t  stencil_pitch;   // pitch in uint8_t elements per row
    bool      is_fbo;          // true if user FBO, false if default BGLDrawable
    bool      has_color;       // true if valid color buffer attached/present
    bool      has_depth;       // true if valid depth buffer attached/present
    bool      has_stencil;     // true if valid stencil buffer attached/present
    GLuint    fbo_id;          // 0 for default framebuffer
    GLTextureObject* color_tex_obj; // for feedback loop hazard detection
} GLRenderTarget;

/* Renderbuffer Object Attachment Storage */
typedef struct {
    GLuint   id;
    bool     allocated;
    GLenum   internal_format;
    GLsizei  width;
    GLsizei  height;
    void*    storage_buffer;   // malloc/aligned buffer pointer
    size_t   storage_size;
} GLRenderbufferObject;

/* Framebuffer Attachment Types */
typedef enum {
    GL_ATTACHMENT_NONE = 0,
    GL_ATTACHMENT_TEXTURE_2D,
    GL_ATTACHMENT_RENDERBUFFER
} GLAttachmentType;

/* Framebuffer Attachment Entry */
typedef struct {
    GLAttachmentType type;
    GLuint           object_id;
    GLint            texture_level;
} GLFramebufferAttachment;

/* Framebuffer Object Structure */
typedef struct {
    GLuint                  id;
    bool                    allocated;
    GLFramebufferAttachment color_attachment0;
    GLFramebufferAttachment depth_attachment;
    GLFramebufferAttachment stencil_attachment;
} GLFramebufferObject;

/* Forward declaration of GLContextState */
struct GLContextState;

/* FBO / RBO Lifecycle & API Functions */
void gl_fbo_system_init(struct GLContextState* state);
void gl_fbo_system_cleanup(struct GLContextState* state);

GLuint gl_fbo_gen_framebuffer(struct GLContextState* state);
bool   gl_fbo_delete_framebuffer(struct GLContextState* state, GLuint id);
GLFramebufferObject* gl_fbo_get_framebuffer(struct GLContextState* state, GLuint id);

GLuint gl_rbo_gen_renderbuffer(struct GLContextState* state);
bool   gl_rbo_delete_renderbuffer(struct GLContextState* state, GLuint id);
GLRenderbufferObject* gl_rbo_get_renderbuffer(struct GLContextState* state, GLuint id);

/* Active Render Target Resolver */
bool gl_get_active_render_target(struct GLContextState* state, GLRenderTarget* out_target);

/* Completeness Validation */
GLenum gl_fbo_check_completeness(struct GLContextState* state, GLuint fbo_id);

/* Feedback Loop Hazard Detection */
bool gl_fbo_is_sampler_aliased(struct GLContextState* state, const GLTextureObject* sampler_tex);

/* Attachment Invalidators */
void gl_fbo_on_texture_deleted(struct GLContextState* state, GLuint texture_id);
void gl_fbo_on_renderbuffer_deleted(struct GLContextState* state, GLuint rbo_id);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_FBO_H
