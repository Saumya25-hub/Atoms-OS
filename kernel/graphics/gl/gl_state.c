#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/core/memory/heap/include/heap.h"

GLContextState* gl_state_create(void) {
    GLContextState* state = (GLContextState*)kmalloc(sizeof(GLContextState));
    if (!state) return NULL;

    state->last_error = GL_NO_ERROR;

    state->clear_color[0] = 0.0f;
    state->clear_color[1] = 0.0f;
    state->clear_color[2] = 0.0f;
    state->clear_color[3] = 0.0f;
    state->clear_depth    = 1.0f;

    state->viewport_x = 0;
    state->viewport_y = 0;
    state->viewport_w = 640;
    state->viewport_h = 480;

    // Depth state defaults
    state->depth_test_enabled = false;
    state->depth_func         = GL_LESS;
    state->depth_writemask    = true;

    // Face culling defaults
    state->cull_face_enabled  = false;
    state->cull_face_mode     = GL_BACK;
    state->front_face_mode    = GL_CCW;

    // Texture 2D defaults
    state->texture_2d_enabled = false;
    state->bound_texture_2d   = 0;
    state->current_texcoord[0] = 0.0f;
    state->current_texcoord[1] = 0.0f;
    state->current_texcoord[2] = 0.0f;
    state->current_texcoord[3] = 1.0f;
    state->texture_env_mode   = GL_MODULATE;
    state->next_texture_id    = 1;

    for (size_t i = 0; i < GL_MAX_TEXTURE_OBJECTS; i++) {
        gl_texture_init_object(&state->textures[i], 0);
    }
    gl_texture_init_object(&state->textures[0], 0);

    // Phase 5 Lighting & Material defaults
    state->lighting_enabled       = false;
    state->normalize_enabled      = false;
    state->rescale_normal_enabled = false;
    state->color_material_enabled = false;
    state->color_material_face    = GL_FRONT_AND_BACK;
    state->color_material_mode    = GL_AMBIENT_AND_DIFFUSE;
    state->shade_model            = GL_SMOOTH;

    state->current_normal[0] = 0.0f;
    state->current_normal[1] = 0.0f;
    state->current_normal[2] = 1.0f;

    gl_lighting_init_material(&state->front_material);
    gl_lighting_init_material(&state->back_material);

    for (GLuint i = 0; i < GL_MAX_LIGHTS; i++) {
        gl_lighting_init_light(&state->lights[i], i);
    }

    state->light_model_ambient[0] = 0.2f;
    state->light_model_ambient[1] = 0.2f;
    state->light_model_ambient[2] = 0.2f;
    state->light_model_ambient[3] = 1.0f;
    state->light_model_local_viewer = false;
    state->light_model_two_side     = false;

    state->matrix_mode = GL_MODELVIEW;
    state->modelview_top = 0;
    gl_matrix_identity(&state->modelview_stack[0]);

    state->projection_top = 0;
    gl_matrix_identity(&state->projection_stack[0]);

    state->current_color[0] = 1.0f; // Default white
    state->current_color[1] = 1.0f;
    state->current_color[2] = 1.0f;
    state->current_color[3] = 1.0f;

    // Phase 6 Defaults
    state->alpha_test_enabled = false;
    state->alpha_func         = GL_ALWAYS;
    state->alpha_ref          = 0.0f;

    state->blend_enabled      = false;
    state->blend_src_factor   = GL_ONE;
    state->blend_dst_factor   = GL_ZERO;

    state->fog_enabled        = false;
    state->fog_mode           = GL_EXP;
    state->fog_color[0]       = 0.0f;
    state->fog_color[1]       = 0.0f;
    state->fog_color[2]       = 0.0f;
    state->fog_color[3]       = 0.0f;
    state->fog_density        = 1.0f;
    state->fog_start          = 0.0f;
    state->fog_end            = 1.0f;

    state->scissor_test_enabled = false;
    state->scissor_x            = 0;
    state->scissor_y            = 0;
    state->scissor_w            = 0;
    state->scissor_h            = 0;

    state->color_mask[0] = true;
    state->color_mask[1] = true;
    state->color_mask[2] = true;
    state->color_mask[3] = true;

    state->depth_range_near = 0.0;
    state->depth_range_far  = 1.0;

    state->point_size = 1.0f;
    state->line_width = 1.0f;

    state->polygon_mode_front = GL_FILL;
    state->polygon_mode_back  = GL_FILL;

    state->in_begin_end = false;
    state->primitive_mode = GL_POINTS;

    state->vertex_capacity = 512;
    state->vertex_count = 0;
    state->vertex_buffer = (GLVertex*)kmalloc(state->vertex_capacity * sizeof(GLVertex));

    state->primitive_capacity = 512;
    state->primitive_count = 0;
    state->primitive_buffer = (GLPrimitive*)kmalloc(state->primitive_capacity * sizeof(GLPrimitive));

    if (!state->vertex_buffer || !state->primitive_buffer) {
        if (state->vertex_buffer) kfree(state->vertex_buffer);
        if (state->primitive_buffer) kfree(state->primitive_buffer);
        kfree(state);
        return NULL;
    }

    state->total_vertices_transformed = 0;
    state->total_primitives_assembled = 0;
    state->rasterizer_triangles_submitted = 0;
    state->rasterizer_fragments_tested = 0;
    state->rasterizer_depth_rejected_fragments = 0;
    state->rasterizer_pixels_written = 0;
    state->texture_uploads = 0;
    state->texture_samples = 0;
    state->nearest_samples = 0;
    state->linear_samples = 0;
    state->vertices_lit = 0;

    // Phase 7 Defaults
    state->vertex_array.enabled = false;
    state->vertex_array.size = 4;
    state->vertex_array.type = GL_FLOAT;
    state->vertex_array.stride = 0;
    state->vertex_array.pointer = NULL;

    state->color_array.enabled = false;
    state->color_array.size = 4;
    state->color_array.type = GL_FLOAT;
    state->color_array.stride = 0;
    state->color_array.pointer = NULL;

    state->normal_array.enabled = false;
    state->normal_array.size = 3;
    state->normal_array.type = GL_FLOAT;
    state->normal_array.stride = 0;
    state->normal_array.pointer = NULL;

    state->texcoord_array.enabled = false;
    state->texcoord_array.size = 4;
    state->texcoord_array.type = GL_FLOAT;
    state->texcoord_array.stride = 0;
    state->texcoord_array.pointer = NULL;

    state->is_compiling_list = false;
    state->compiling_list_id = 0;
    state->compiling_list_mode = GL_COMPILE;
    state->list_call_depth = 0;
    for (size_t i = 0; i < GL_MAX_LISTS; i++) {
        state->display_lists[i] = NULL;
    }

    state->draw_calls = 0;
    state->vertices_submitted = 0;
    state->indices_processed = 0;
    state->display_list_calls = 0;

    // Phase 8 Defaults
    state->unpack_alignment = 4;
    state->pack_alignment = 4;
    state->mipmap_generations = 0;
    state->read_pixels_calls = 0;
    state->copy_texture_calls = 0;

    // Phase 9 Defaults
    state->stencil_test_enabled  = false;
    state->stencil_func          = GL_ALWAYS;
    state->stencil_ref           = 0;
    state->stencil_value_mask    = 0xFFFFFFFF;
    state->stencil_write_mask    = 0xFFFFFFFF;
    state->stencil_fail_op       = GL_KEEP;
    state->stencil_depth_fail_op = GL_KEEP;
    state->stencil_depth_pass_op = GL_KEEP;
    state->clear_stencil         = 0;

    state->polygon_offset_fill_enabled  = false;
    state->polygon_offset_line_enabled  = false;
    state->polygon_offset_point_enabled = false;
    state->polygon_offset_factor        = 0.0f;
    state->polygon_offset_units         = 0.0f;

    state->fragments_stencil_rejected = 0;
    state->stencil_updates            = 0;
    state->depth_writes               = 0;
    state->culled_triangles           = 0;
    state->polygon_offset_fragments   = 0;

    // Phase 10 FBO/RBO Init & Telemetry
    gl_fbo_system_init(state);

    state->framebuffer_binds                = 0;
    state->render_target_switches           = 0;
    state->offscreen_frames                 = 0;
    state->framebuffer_completeness_checks  = 0;
    state->framebuffer_incomplete_attempts  = 0;
    state->render_to_texture_passes         = 0;
    state->feedback_loop_hazards            = 0;

    return state;
}

void gl_state_destroy(GLContextState* state) {
    if (!state) return;

    gl_fbo_system_cleanup(state);

    for (size_t i = 0; i < GL_MAX_TEXTURE_OBJECTS; i++) {
        gl_texture_free_object(&state->textures[i]);
    }

    if (state->vertex_buffer) {
        kfree(state->vertex_buffer);
        state->vertex_buffer = NULL;
    }

    if (state->primitive_buffer) {
        kfree(state->primitive_buffer);
        state->primitive_buffer = NULL;
    }

    kfree(state);
}

GLContextState* gl_state_get_current(void) {
    BGLContext* ctx = bglGetCurrentContext();
    if (!ctx) return NULL;
    return (GLContextState*)ctx->private_data;
}

void gl_state_set_error(GLContextState* state, GLenum err) {
    if (!state) return;
    if (state->last_error == GL_NO_ERROR) {
        state->last_error = err;
    }
}

GLMatrix4x4* gl_state_get_current_matrix(GLContextState* state) {
    if (!state) return NULL;
    if (state->matrix_mode == GL_MODELVIEW) {
        return &state->modelview_stack[state->modelview_top];
    } else if (state->matrix_mode == GL_PROJECTION) {
        return &state->projection_stack[state->projection_top];
    }
    return NULL;
}

GLTextureObject* gl_state_get_texture(GLContextState* state, GLuint id) {
    if (!state) return NULL;
    for (size_t i = 0; i < GL_MAX_TEXTURE_OBJECTS; i++) {
        if (state->textures[i].id == id && (id == 0 || state->textures[i].allocated)) {
            return &state->textures[i];
        }
    }
    return NULL;
}

GLTextureObject* gl_state_get_bound_texture(GLContextState* state) {
    if (!state) return NULL;
    return gl_state_get_texture(state, state->bound_texture_2d);
}
