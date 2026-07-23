#ifndef ATOMS_OS_GL_STATE_H
#define ATOMS_OS_GL_STATE_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"
#include "kernel/graphics/gl/gl_math.h"
#include "kernel/graphics/gl/gl_pipeline.h"
#include "kernel/graphics/gl/gl_texture.h"
#include "kernel/graphics/gl/gl_lighting.h"
#include "kernel/graphics/gl/gl_fbo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GLContextState {
    GLenum    last_error;

    GLclampf  clear_color[4];
    GLclampf  clear_depth;
    GLint     viewport_x;
    GLint     viewport_y;
    GLsizei   viewport_w;
    GLsizei   viewport_h;

    // Depth Test State
    bool      depth_test_enabled;
    GLenum    depth_func;
    bool      depth_writemask;

    // Face Culling State
    bool      cull_face_enabled;
    GLenum    cull_face_mode;
    GLenum    front_face_mode;

    // Texture 2D State
    bool            texture_2d_enabled;
    GLuint          bound_texture_2d;
    GLfloat         current_texcoord[4];
    GLenum          texture_env_mode;
    GLTextureObject textures[GL_MAX_TEXTURE_OBJECTS];
    GLuint          next_texture_id;

    // Lighting & Material State
    bool            lighting_enabled;
    bool            normalize_enabled;
    bool            rescale_normal_enabled;
    bool            color_material_enabled;
    GLenum          color_material_face;
    GLenum          color_material_mode;
    GLenum          shade_model;
    GLMaterialState front_material;
    GLMaterialState back_material;
    GLLightState    lights[GL_MAX_LIGHTS];
    GLfloat         light_model_ambient[4];
    bool            light_model_local_viewer;
    bool            light_model_two_side;
    GLfloat         current_normal[3];

    GLenum      matrix_mode;
    GLMatrix4x4 modelview_stack[GL_MAX_MODELVIEW_STACK_DEPTH];
    int         modelview_top;
    GLMatrix4x4 projection_stack[GL_MAX_PROJECTION_STACK_DEPTH];
    int         projection_top;

    GLfloat   current_color[4];

    bool         in_begin_end;
    GLenum       primitive_mode;
    GLVertex*    vertex_buffer;
    uint32_t     vertex_count;
    uint32_t     vertex_capacity;

    GLPrimitive* primitive_buffer;
    uint32_t     primitive_count;
    uint32_t     primitive_capacity;

    // Telemetry Counters
    uint64_t     total_vertices_transformed;
    uint64_t     total_primitives_assembled;
    uint64_t     rasterizer_triangles_submitted;
    uint64_t     rasterizer_fragments_tested;
    uint64_t     rasterizer_depth_rejected_fragments;
    uint64_t     rasterizer_pixels_written;
    uint64_t     texture_uploads;
    uint64_t     texture_samples;
    uint64_t     nearest_samples;
    uint64_t     linear_samples;
    uint64_t     vertices_lit;

    // Phase 6 Fragment & Rasterization State
    bool         alpha_test_enabled;
    GLenum       alpha_func;
    GLclampf     alpha_ref;

    bool         blend_enabled;
    GLenum       blend_src_factor;
    GLenum       blend_dst_factor;

    bool         fog_enabled;
    GLenum       fog_mode;
    GLclampf     fog_color[4];
    GLfloat      fog_density;
    GLfloat      fog_start;
    GLfloat      fog_end;

    bool         scissor_test_enabled;
    GLint        scissor_x;
    GLint        scissor_y;
    GLsizei      scissor_w;
    GLsizei      scissor_h;

    bool         color_mask[4];

    GLclampd     depth_range_near;
    GLclampd     depth_range_far;

    GLfloat      point_size;
    GLfloat      line_width;

    GLenum       polygon_mode_front;
    GLenum       polygon_mode_back;

    // Phase 6 Telemetry Counters
    uint64_t     fragments_generated;
    uint64_t     fragments_scissor_rejected;
    uint64_t     fragments_alpha_rejected;
    uint64_t     fragments_depth_rejected;
    uint64_t     fragments_blended;
    uint64_t     fragments_written;

    // Phase 7 Client Array State
    struct {
        bool        enabled;
        GLint       size;
        GLenum      type;
        GLsizei     stride;
        const void* pointer;
    } vertex_array, color_array, normal_array, texcoord_array;

    // Phase 7 Telemetry
    uint64_t     draw_calls;
    uint64_t     vertices_submitted;
    uint64_t     indices_processed;
    uint64_t     display_list_calls;

    // Phase 7 Display Lists State
    bool         is_compiling_list;
    GLuint       compiling_list_id;
    GLenum       compiling_list_mode;
    uint32_t     list_call_depth;
    void*        display_lists[GL_MAX_LISTS];

    // Phase 8 Pixel Storage & Telemetry
    GLint        unpack_alignment;
    GLint        pack_alignment;
    uint64_t     mipmap_generations;
    uint64_t     read_pixels_calls;
    uint64_t     copy_texture_calls;

    // Phase 9 Stencil & Polygon Offset State
    bool         stencil_test_enabled;
    GLenum       stencil_func;
    GLint        stencil_ref;
    GLuint       stencil_value_mask;
    GLuint       stencil_write_mask;
    GLenum       stencil_fail_op;
    GLenum       stencil_depth_fail_op;
    GLenum       stencil_depth_pass_op;
    GLint        clear_stencil;

    bool         polygon_offset_fill_enabled;
    bool         polygon_offset_line_enabled;
    bool         polygon_offset_point_enabled;
    GLfloat      polygon_offset_factor;
    GLfloat      polygon_offset_units;

    // Phase 9 Telemetry
    uint64_t     fragments_stencil_rejected;
    uint64_t     stencil_updates;
    uint64_t     depth_writes;
    uint64_t     culled_triangles;
    uint64_t     polygon_offset_fragments;

    // Phase 10 Framebuffer & Renderbuffer Objects
    GLuint                   bound_draw_framebuffer;
    GLuint                   bound_read_framebuffer;
    GLuint                   bound_renderbuffer;
    GLFramebufferObject      framebuffers[GL_MAX_FRAMEBUFFERS];
    GLRenderbufferObject     renderbuffers[GL_MAX_RENDERBUFFERS];

    // Phase 10 Telemetry Counters
    uint64_t                 framebuffer_binds;
    uint64_t                 render_target_switches;
    uint64_t                 offscreen_frames;
    uint64_t                 framebuffer_completeness_checks;
    uint64_t                 framebuffer_incomplete_attempts;
    uint64_t                 render_to_texture_passes;
    uint64_t                 feedback_loop_hazards;
} GLContextState;

GLContextState*  gl_state_create(void);
void             gl_state_destroy(GLContextState* state);
GLContextState*  gl_state_get_current(void);
void             gl_state_set_error(GLContextState* state, GLenum err);
GLMatrix4x4*     gl_state_get_current_matrix(GLContextState* state);
GLTextureObject* gl_state_get_texture(GLContextState* state, GLuint id);
GLTextureObject* gl_state_get_bound_texture(GLContextState* state);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_STATE_H
