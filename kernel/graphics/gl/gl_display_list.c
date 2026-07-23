#include "kernel/graphics/gl/gl_display_list.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char*);

GLuint glGenLists(GLsizei range) {
    GLContextState* state = gl_state_get_current();
    if (!state || range <= 0 || range > GL_MAX_LISTS) {
        if (state) gl_state_set_error(state, GL_INVALID_VALUE);
        return 0;
    }

    // Find continuous range of free list slots
    for (GLuint i = 1; i <= GL_MAX_LISTS - (GLuint)range; i++) {
        bool free_block = true;
        for (GLsizei j = 0; j < range; j++) {
            if (state->display_lists[i + j] != NULL) {
                free_block = false;
                break;
            }
        }
        if (free_block) {
            // Mark placeholder non-null pointers
            for (GLsizei j = 0; j < range; j++) {
                GLDisplayList* dl = (GLDisplayList*)kmalloc(sizeof(GLDisplayList));
                if (dl) {
                    dl->id = i + j;
                    dl->active = true;
                    dl->commands = NULL;
                    dl->command_count = 0;
                    dl->command_capacity = 0;
                    state->display_lists[i + j] = dl;
                }
            }
            return i;
        }
    }

    gl_state_set_error(state, GL_OUT_OF_MEMORY);
    return 0;
}

static void free_display_list_commands(GLDisplayList* dl) {
    if (!dl || !dl->commands) return;
    for (uint32_t i = 0; i < dl->command_count; i++) {
        GLCommand* cmd = &dl->commands[i];
        if (cmd->type == GL_CMD_TEX_IMAGE_2D && cmd->data.tex_image_2d.pixel_payload) {
            kfree(cmd->data.tex_image_2d.pixel_payload);
            cmd->data.tex_image_2d.pixel_payload = NULL;
        } else if (cmd->type == GL_CMD_TEX_SUB_IMAGE_2D && cmd->data.tex_sub_image_2d.pixel_payload) {
            kfree(cmd->data.tex_sub_image_2d.pixel_payload);
            cmd->data.tex_sub_image_2d.pixel_payload = NULL;
        }
    }
    kfree(dl->commands);
    dl->commands = NULL;
    dl->command_count = 0;
    dl->command_capacity = 0;
}

void glNewList(GLuint list, GLenum mode) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (list == 0 || list >= GL_MAX_LISTS) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE) {
        gl_state_set_error(state, GL_INVALID_ENUM);
        return;
    }

    if (state->is_compiling_list || state->in_begin_end) {
        gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    GLDisplayList* dl = (GLDisplayList*)state->display_lists[list];
    if (!dl) {
        dl = (GLDisplayList*)kmalloc(sizeof(GLDisplayList));
        if (!dl) {
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        dl->id = list;
        dl->active = true;
        dl->commands = NULL;
        dl->command_count = 0;
        dl->command_capacity = 0;
        state->display_lists[list] = dl;
    } else {
        free_display_list_commands(dl);
    }

    state->is_compiling_list = true;
    state->compiling_list_id = list;
    state->compiling_list_mode = mode;
}

void glEndList(void) {
    GLContextState* state = gl_state_get_current();
    if (!state || !state->is_compiling_list) {
        if (state) gl_state_set_error(state, GL_INVALID_OPERATION);
        return;
    }

    state->is_compiling_list = false;
    state->compiling_list_id = 0;
}

bool gl_display_list_is_compiling(GLContextState* state) {
    return state ? state->is_compiling_list : false;
}

void gl_display_list_record_command(GLContextState* state, const GLCommand* cmd) {
    if (!state || !state->is_compiling_list || !cmd) return;

    GLuint list_id = state->compiling_list_id;
    if (list_id == 0 || list_id >= GL_MAX_LISTS) return;

    GLDisplayList* dl = (GLDisplayList*)state->display_lists[list_id];
    if (!dl) return;

    if (dl->command_count >= dl->command_capacity) {
        uint32_t new_cap = (dl->command_capacity == 0) ? 16 : (dl->command_capacity * 2);
        GLCommand* new_cmds = (GLCommand*)kmalloc(new_cap * sizeof(GLCommand));
        if (!new_cmds) {
            gl_state_set_error(state, GL_OUT_OF_MEMORY);
            return;
        }
        for (uint32_t i = 0; i < dl->command_count; i++) {
            new_cmds[i] = dl->commands[i];
        }
        if (dl->commands) kfree(dl->commands);
        dl->commands = new_cmds;
        dl->command_capacity = new_cap;
    }

    dl->commands[dl->command_count++] = *cmd;
}

void glCallList(GLuint list) {
    GLContextState* state = gl_state_get_current();
    if (!state) return;

    if (list == 0 || list >= GL_MAX_LISTS) {
        gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    if (state->is_compiling_list) {
        GLCommand cmd;
        cmd.type = GL_CMD_CALL_LIST;
        cmd.data.call_list.list = list;
        gl_display_list_record_command(state, &cmd);
        if (state->compiling_list_mode == GL_COMPILE) return;
    }

    GLDisplayList* dl = (GLDisplayList*)state->display_lists[list];
    if (!dl || !dl->active || !dl->commands) return;

    if (state->list_call_depth >= GL_MAX_LIST_CALL_DEPTH) {
        gl_state_set_error(state, GL_STACK_OVERFLOW);
        return;
    }

    state->list_call_depth++;
    state->display_list_calls++;

    // Execute recorded commands
    for (uint32_t i = 0; i < dl->command_count; i++) {
        const GLCommand* cmd = &dl->commands[i];
        switch (cmd->type) {
            case GL_CMD_BEGIN:
                glBegin(cmd->data.begin.mode);
                break;
            case GL_CMD_END:
                glEnd();
                break;
            case GL_CMD_VERTEX4F:
                glVertex4f(cmd->data.vertex.x, cmd->data.vertex.y, cmd->data.vertex.z, cmd->data.vertex.w);
                break;
            case GL_CMD_COLOR4F:
                glColor4f(cmd->data.color.r, cmd->data.color.g, cmd->data.color.b, cmd->data.color.a);
                break;
            case GL_CMD_NORMAL3F:
                glNormal3f(cmd->data.normal.x, cmd->data.normal.y, cmd->data.normal.z);
                break;
            case GL_CMD_TEXCOORD4F:
                glTexCoord4f(cmd->data.texcoord.s, cmd->data.texcoord.t, cmd->data.texcoord.r, cmd->data.texcoord.q);
                break;
            case GL_CMD_ENABLE:
                glEnable(cmd->data.enable.cap);
                break;
            case GL_CMD_DISABLE:
                glDisable(cmd->data.disable.cap);
                break;
            case GL_CMD_MATRIX_MODE:
                glMatrixMode(cmd->data.matrix_mode.mode);
                break;
            case GL_CMD_LOAD_IDENTITY:
                glLoadIdentity();
                break;
            case GL_CMD_TRANSLATEF:
                glTranslatef(cmd->data.translate.x, cmd->data.translate.y, cmd->data.translate.z);
                break;
            case GL_CMD_ROTATEF:
                glRotatef(cmd->data.rotate.angle, cmd->data.rotate.x, cmd->data.rotate.y, cmd->data.rotate.z);
                break;
            case GL_CMD_SCALEF:
                glScalef(cmd->data.scale.x, cmd->data.scale.y, cmd->data.scale.z);
                break;
            case GL_CMD_LIGHTFV:
                glLightfv(cmd->data.light.light, cmd->data.light.pname, cmd->data.light.params);
                break;
            case GL_CMD_MATERIALFV:
                glMaterialfv(cmd->data.material.face, cmd->data.material.pname, cmd->data.material.params);
                break;
            case GL_CMD_CALL_LIST:
                glCallList(cmd->data.call_list.list);
                break;
            case GL_CMD_TEX_IMAGE_2D:
                glTexImage2D(GL_TEXTURE_2D, cmd->data.tex_image_2d.level, cmd->data.tex_image_2d.internalformat, cmd->data.tex_image_2d.width, cmd->data.tex_image_2d.height, cmd->data.tex_image_2d.border, cmd->data.tex_image_2d.format, cmd->data.tex_image_2d.type, cmd->data.tex_image_2d.pixel_payload);
                break;
            case GL_CMD_TEX_SUB_IMAGE_2D:
                glTexSubImage2D(GL_TEXTURE_2D, cmd->data.tex_sub_image_2d.level, cmd->data.tex_sub_image_2d.xoffset, cmd->data.tex_sub_image_2d.yoffset, cmd->data.tex_sub_image_2d.width, cmd->data.tex_sub_image_2d.height, cmd->data.tex_sub_image_2d.format, cmd->data.tex_sub_image_2d.type, cmd->data.tex_sub_image_2d.pixel_payload);
                break;
            case GL_CMD_TEX_PARAMETERI:
                glTexParameteri(GL_TEXTURE_2D, cmd->data.tex_param.pname, cmd->data.tex_param.param);
                break;
            case GL_CMD_GENERATE_MIPMAP:
                glGenerateMipmap(GL_TEXTURE_2D);
                break;
            case GL_CMD_PIXEL_STOREI:
                glPixelStorei(cmd->data.pixel_store.pname, cmd->data.pixel_store.param);
                break;
            case GL_CMD_STENCIL_FUNC:
                glStencilFunc(cmd->data.stencil_func.func, cmd->data.stencil_func.ref, cmd->data.stencil_func.mask);
                break;
            case GL_CMD_STENCIL_MASK:
                glStencilMask(cmd->data.stencil_mask.mask);
                break;
            case GL_CMD_STENCIL_OP:
                glStencilOp(cmd->data.stencil_op.sfail, cmd->data.stencil_op.dpfail, cmd->data.stencil_op.dppass);
                break;
            case GL_CMD_CLEAR_STENCIL:
                glClearStencil(cmd->data.clear_stencil.s);
                break;
            case GL_CMD_DEPTH_MASK:
                glDepthMask(cmd->data.depth_mask.flag);
                break;
            case GL_CMD_FRONT_FACE:
                glFrontFace(cmd->data.front_face.mode);
                break;
            case GL_CMD_CULL_FACE:
                glCullFace(cmd->data.cull_face.mode);
                break;
            case GL_CMD_POLYGON_OFFSET:
                glPolygonOffset(cmd->data.polygon_offset.factor, cmd->data.polygon_offset.units);
                break;
            default:
                break;
        }
    }

    state->list_call_depth--;
}

void glDeleteLists(GLuint list, GLsizei range) {
    GLContextState* state = gl_state_get_current();
    if (!state || list == 0 || range <= 0) {
        if (state) gl_state_set_error(state, GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < range; i++) {
        GLuint id = list + (GLuint)i;
        if (id < GL_MAX_LISTS && state->display_lists[id]) {
            GLDisplayList* dl = (GLDisplayList*)state->display_lists[id];
            free_display_list_commands(dl);
            kfree(dl);
            state->display_lists[id] = NULL;
        }
    }
}

GLboolean glIsList(GLuint list) {
    GLContextState* state = gl_state_get_current();
    if (!state || list == 0 || list >= GL_MAX_LISTS) return GL_FALSE;
    return (state->display_lists[list] != NULL) ? GL_TRUE : GL_FALSE;
}
