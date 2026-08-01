// Engine 10: Shader Manager
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"

typedef struct {
    AGPShaderType type;
    bool          compiled;
    bool          in_use;
} AGPShaderInternal;

typedef struct {
    AGPShaderID vs;
    AGPShaderID fs;
    bool        linked;
    bool        in_use;
} AGPProgramInternal;

static AGPShaderInternal  s_shader_pool[AGP_MAX_SHADERS];
static AGPProgramInternal s_program_pool[AGP_MAX_PROGRAMS];

AGPShaderID AGP_CreateShader(AGPShaderType type, const char* source) {
    (void)source;
    for (uint32_t i = 0; i < AGP_MAX_SHADERS; i++) {
        if (!s_shader_pool[i].in_use) {
            s_shader_pool[i].in_use = true;
            s_shader_pool[i].type = type;
            s_shader_pool[i].compiled = true;
            return i + 1;
        }
    }
    return 0;
}

AGPProgramID AGP_CreateProgram(AGPShaderID vs, AGPShaderID fs) {
    for (uint32_t i = 0; i < AGP_MAX_PROGRAMS; i++) {
        if (!s_program_pool[i].in_use) {
            s_program_pool[i].in_use = true;
            s_program_pool[i].vs = vs;
            s_program_pool[i].fs = fs;
            s_program_pool[i].linked = true;
            return i + 1;
        }
    }
    return 0;
}

void AGP_UseProgram(AGPProgramID prog_id) {
    AGPContext* ctx = AGP_GetCurrentContext();
    if (ctx) ctx->active_program = prog_id;
}

void AGP_DeleteShader(AGPShaderID shader_id) {
    if (shader_id == 0 || shader_id > AGP_MAX_SHADERS) return;
    uint32_t idx = shader_id - 1;
    s_shader_pool[idx].in_use = false;
}
