#ifndef AGP_API_H
#define AGP_API_H

#include "agp_types.h"

// Subsystem Lifecycle
int32_t       AGP_Init(void);
void          AGP_Shutdown(void);

// Context Management
AGPContextID  AGP_CreateContext(uint32_t win_id, uint32_t w, uint32_t h);
void          AGP_DestroyContext(AGPContextID ctx_id);
int32_t       AGP_MakeCurrent(AGPContextID ctx_id);
AGPContext*   AGP_GetCurrentContext(void);

// Surface & Swapchain
AGPSurface*   AGP_CreateSurface(uint32_t w, uint32_t h, AGPFormat fmt);
void          AGP_DestroySurface(AGPSurface* surface);
int32_t       AGP_SwapBuffers(AGPContextID ctx_id);

// Display & Window
void          AGP_SetViewport(int32_t x, int32_t y, uint32_t w, uint32_t h);
void          AGP_Clear(uint32_t color, float depth);

// Resource Management: Textures
AGPTextureID  AGP_CreateTexture(uint32_t w, uint32_t h, AGPFormat fmt, const void* pixels);
void          AGP_BindTexture(AGPTextureID tex_id);
void          AGP_DeleteTexture(AGPTextureID tex_id);

// Resource Management: Buffers
AGPBufferID   AGP_CreateBuffer(AGPBufferType type, size_t size, const void* data);
void          AGP_BindBuffer(AGPBufferType type, AGPBufferID buf_id);
void          AGP_DeleteBuffer(AGPBufferID buf_id);

// Resource Management: Shaders & Programs
AGPShaderID   AGP_CreateShader(AGPShaderType type, const char* source);
AGPProgramID  AGP_CreateProgram(AGPShaderID vs, AGPShaderID fs);
void          AGP_UseProgram(AGPProgramID prog_id);
void          AGP_DeleteShader(AGPShaderID shader_id);

// Command Submission & Drawing
void          AGP_Begin(AGPPrimitiveType prim);
void          AGP_Vertex3f(float x, float y, float z);
void          AGP_Color4f(float r, float g, float b, float a);
void          AGP_TexCoord2f(float u, float v);
void          AGP_End(void);
void          AGP_DrawArrays(AGPPrimitiveType prim, uint32_t first, uint32_t count);

// Diagnostics & Synchronization
void          AGP_WaitGPU(void);
void          AGP_GetDiagnostics(AGPDiagnostics* out_diag);
void          agp_run_certification_suite(void);

#endif // AGP_API_H
