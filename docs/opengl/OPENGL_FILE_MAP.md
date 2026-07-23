# OPENGL SOURCE FILE MAP — ATOMS OS GRAPHICS ARCHITECTURE

This document provides a precise mapping of all source files comprising the ATOMS OS OpenGL/BGL graphics subsystem and ATOMS GRAPH 3D application.

---

## 1. Public OpenGL API Subsystem

### `kernel/graphics/gl/gl.h`
- **Purpose**: Public C header declaring all standard `gl*` entry points, constants, and data types.
- **Important Functions/Structures**: `glBegin`, `glEnd`, `glVertex3f`, `glColor4f`, `glTexCoord2f`, `glNormal3f`, `glMatrixMode`, `glLoadIdentity`, `glRotatef`, `glTranslatef`, `glScalef`, `glFrustum`, `glViewport`, `glEnable`, `glDisable`, `glGenTextures`, `glBindTexture`, `glTexImage2D`, `glGenFramebuffers`, `glBindFramebuffer`, `glGenRenderbuffers`, etc.
- **Who Calls It**: Applications (`atoms_graph_scene.c`, DOOM `i_video.c`), verification test suites (`test_gl_phase*.c`).
- **What It Depends On**: `kernel/graphics/gl/gl_types.h`, `gl_constants.h`.
- **When to Read/Modify**: When adding new standard OpenGL API function prototypes or GL enum constants.

### `kernel/graphics/gl/gl.c`
- **Purpose**: Public API dispatcher implementation connecting external `gl*` calls to the active context state (`GLContextState`).
- **Important Functions**: Implementation of `glBegin`, `glEnd`, `glVertex3f`, `glColor3f`, `glTexCoord2f`, `glNormal3f`, `glMatrixMode`, `glLoadIdentity`, `glRotatef`, `glFrustum`, `glEnable`, `glDisable`, `glClear`, `glClearColor`, `glViewport`, `glDrawElements`.
- **Who Calls It**: Applications and test suites calling standard GL functions.
- **What It Depends On**: `gl_state.h`, `gl_pipeline.h`, `gl_rasterizer.h`, `gl_texture.h`, `gl_fbo.h`.
- **When to Read/Modify**: When implementing new public OpenGL functions or adjusting parameter validation for public GL calls.

### `kernel/graphics/gl/gl_constants.h`
- **Purpose**: Standard OpenGL enumeration constants (e.g. `GL_TRIANGLES`, `GL_MODELVIEW`, `GL_PROJECTION`, `GL_TEXTURE_2D`, `GL_FRAMEBUFFER`, `GL_COLOR_ATTACHMENT0`, `GL_DEPTH_COMPONENT16`, `GL_STENCIL_BUFFER_BIT`).
- **Who Calls It**: `gl.h`, `gl_state.h`, all GL modules and applications.
- **When to Read/Modify**: When adding missing GL macro definitions or enumerations.

---

## 2. GL Context & State Management

### `kernel/graphics/gl/gl_state.h`
- **Purpose**: Core structure definition for `GLContextState` representing all active GL state parameters for a rendering context.
- **Important Structures**: `GLContextState` (matrix stacks, current color, current texcoord, current normal, lighting state, texture bindings, FBO bindings, viewport rect, scissor rect, stencil state, blend state).
- **Who Calls It**: `gl_state.c`, `bgl_context.c`, rasterizer, pipeline modules.
- **When to Read/Modify**: When adding new GL state variables (e.g. additional texture units, new blend modes, or light sources).

### `kernel/graphics/gl/gl_state.c`
- **Purpose**: Allocation, initialization, and destruction of `GLContextState` objects (`gl_state_create`, `gl_state_destroy`, `gl_state_get_current`).
- **Who Calls It**: `bglCreateContext`, `bglDestroyContext`, `bglMakeCurrent`.
- **What It Depends On**: `kernel/core/memory/heap/include/heap.h`.
- **When to Read/Modify**: When modifying default GL context state initialization values or debugging context memory leaks.

---

## 3. Pipeline Assembly & Rasterization

### `kernel/graphics/gl/gl_pipeline.c` & `gl_pipeline.h`
- **Purpose**: Geometry processing pipeline: vertex transformation by MVP matrix, normal transformation, frustum clipping, perspective divide, and viewport mapping.
- **Important Functions**: `gl_pipeline_process_vertex`, `gl_pipeline_assemble_primitive`.
- **Who Calls It**: `gl.c` (`glEnd`, `glDrawArrays`, `glDrawElements`).
- **What It Depends On**: `gl_math.h`, `gl_clip.h`, `gl_viewport.h`, `gl_lighting.h`.
- **When to Read/Modify**: When altering vertex transformation logic, clipping behavior, or primitive assembly rules.

### `kernel/graphics/gl/gl_rasterizer.c` & `gl_rasterizer.h`
- **Purpose**: Software triangle rasterization core. Computes edge functions, bounding boxes, sub-pixel steps, and interpolates per-fragment attributes.
- **Important Functions**: `gl_rasterize_triangle`, `gl_rasterize_line`, `gl_rasterize_point`.
- **Who Calls It**: `gl_pipeline.c`.
- **What It Depends On**: `gl_fragment.h`, `gl_depth.h`, `gl_sampler.h`.
- **When to Read/Modify**: When optimizing triangle rendering performance, fixing visual artifacts, or adjusting barycentric interpolation.

### `kernel/graphics/gl/gl_fragment.c` & `gl_fragment.h`
- **Purpose**: Per-fragment processing pipeline: depth test, stencil test, texture sampling, lighting calculation, alpha test, and alpha blending.
- **Important Functions**: `gl_process_fragment`, `gl_test_depth`, `gl_test_stencil`, `gl_apply_blend`.
- **Who Calls It**: `gl_rasterizer.c`.
- **What It Depends On**: `gl_texture.h`, `gl_depth.h`, `gl_fbo.h`.
- **When to Read/Modify**: When modifying fragment operations (e.g. blend factors, depth comparison functions, or stencil operations).

### `kernel/graphics/gl/gl_math.c` & `gl_math.h`
- **Purpose**: 3D math operations: 4x4 matrix multiplication, matrix inversion, vector dot/cross products, normalization, rotation/translation/scale matrix generation.
- **Important Functions**: `mat4_multiply`, `mat4_perspective`, `mat4_frustum`, `vec3_normalize`, `vec3_cross`.
- **Who Calls It**: `gl.c`, `gl_pipeline.c`, `gl_lighting.c`, `atoms_graph_scene.c`.
- **When to Read/Modify**: When fixing matrix math bugs, projection math, or adding 3D vector utilities.

---

## 4. Texture & FBO Subsystems

### `kernel/graphics/gl/gl_texture.c` & `gl_texture.h`
- **Purpose**: Texture object lifecycle (`glGenTextures`, `glDeleteTextures`), pixel payload upload (`glTexImage2D`, `glTexSubImage2D`), and mipmap generation (`glGenerateMipmap`).
- **Important Functions**: `gl_texture_create`, `gl_texture_upload`, `gl_texture_generate_mipmaps`.
- **Who Calls It**: `gl.c`, `atoms_graph_scene.c`, verification tests.
- **What It Depends On**: `heap.h`, `gl_sampler.h`.
- **When to Read/Modify**: When modifying texture allocation, mipmap pyramid creation, or texture sub-image updates.

### `kernel/graphics/gl/gl_sampler.c` & `gl_sampler.h`
- **Purpose**: Texture pixel lookup and filtering routines (nearest, bilinear, trilinear mipmap filtering).
- **Important Functions**: `gl_sample_texture_2d`, `gl_sample_bilinear`, `gl_sample_trilinear`.
- **Who Calls It**: `gl_fragment.c`.
- **When to Read/Modify**: When adding new texture filtering algorithms or fixing texture coordinate wrapping (`GL_REPEAT`, `GL_CLAMP`).

### `kernel/graphics/gl/gl_fbo.c` & `gl_fbo.h`
- **Purpose**: Offscreen Framebuffer Objects (FBO) and Renderbuffer Objects (RBO) state, attachment binding, completeness validation, and target switching.
- **Important Functions**: `gl_fbo_create`, `gl_fbo_bind`, `gl_fbo_attach_texture`, `gl_fbo_attach_renderbuffer`, `gl_fbo_check_completeness`.
- **Who Calls It**: `gl.c` (`glBindFramebuffer`, `glFramebufferTexture2D`, etc.), `atoms_graph_scene.c`.
- **What It Depends On**: `gl_texture.h`, `heap.h`.
- **When to Read/Modify**: When extending FBO attachments, fixing render target switching, or debugging offscreen rendering.

---

## 5. BGL Layer (Context & Drawable Binding)

### `kernel/graphics/bgl/bgl_context.c` & `bgl_context.h`
- **Purpose**: BGL context management pool (`BGLContext`). Associates a GL state machine instance with a target `BGLDrawable`.
- **Important Functions**: `bglCreateContext`, `bglDestroyContext`, `bglMakeCurrent`, `bglReleaseCurrent`.
- **Who Calls It**: Window applications (`atoms_graph_renderer.c`), BGL tests.
- **What It Depends On**: `gl_state.h`, `task.h`.
- **When to Read/Modify**: When modifying context-to-thread/task binding or managing BGL context pools.

### `kernel/graphics/bgl/bgl_drawable.c` & `bgl_drawable.h`
- **Purpose**: Allocation of window client render targets (`BGLDrawable`). Allocates 32-bpp color buffer, 32-bit float depth buffer, and 8-bit stencil buffer.
- **Important Functions**: `bglCreateDrawableForWindow`, `bglDestroyDrawable`, `bglResizeDrawable`.
- **Who Calls It**: `atoms_graph_renderer.c`, test suites.
- **What It Depends On**: `bwe.h`, `heap.h`.
- **When to Read/Modify**: When altering window client area geometry insets or buffer allocation strategies.

### `kernel/graphics/bgl/bgl.c` & `bgl.h`
- **Purpose**: Top-level BGL wrapper functions including buffer swapping (`bglSwapBuffers`).
- **Important Functions**: `bglSwapBuffers`, `bglGetProcAddress`.
- **Who Calls It**: `atoms_graph_renderer.c`, window render loops.
- **What It Depends On**: `bgl_context.h`, `bgl_drawable.h`.
- **When to Read/Modify**: When adjusting front/back buffer presentation or window invalidation signals.

---

## 6. Window Manager & Shell Integration

### `kernel/wm/bwe/src/bwe_window.c` & `bwe.h`
- **Purpose**: BOSurface Window Engine (BWE V2.0) window manager core. Creates window surfaces, processes Z-order, handles focus, dispatches input events, and destroys windows.
- **Important Functions**: `BOS_CreateSurface`, `BOS_DestroySurface`, `BOS_CreateWindow`, `BWE_DestroyWindow`, `BWE_BringToFront`.
- **Safety Critical Code**: Line ~324 in `BOS_DestroySurface` / `BWE_DestroyWindow`:
  `if (win->user_data && (uintptr_t)win->user_data > 4096) kfree(win->user_data);`
- **Who Calls It**: All OS desktop applications, `horse_engine.c`, `atoms_graph_3d.c`.
- **When to Read/Modify**: When modifying window surface creation, event callbacks, or window destruction safety.

### `kernel/engine/horse_engine.c` & `horse_engine.h`
- **Purpose**: Central ATOMS OS application registry. Registers desktop app metadata, titles, icon paths, and launch handlers.
- **Important Constants**: `APP_ID_GRAPH_3D = 13`.
- **Who Calls It**: Desktop shell (`desktop_shell.c`), start menu (`start_menu.c`), taskbar (`task_panel.c`).
- **When to Read/Modify**: When registering new desktop applications or modifying application launch behavior.

---

## 7. ATOMS GRAPH 3D Application Stack

All modules reside under `kernel/apps/atoms_graph_3d/`:

| File Path | Module Purpose | Primary Functions |
| :--- | :--- | :--- |
| `atoms_graph_3d.c / .h` | App Window Lifecycle | `atoms_graph_3d_launch`, `atoms_graph_3d_close`, `atoms_graph_3d_pump_frame` |
| `atoms_graph_benchmark.c / .h` | Benchmark Controller | `atoms_graph_benchmark_init`, `atoms_graph_benchmark_start`, `atoms_graph_benchmark_step` |
| `atoms_graph_renderer.c / .h` | Renderer BGL Binding | `atoms_graph_renderer_init`, `atoms_graph_renderer_cleanup`, `atoms_graph_renderer_render_frame` |
| `atoms_graph_scene.c / .h` | 10 Procedural 3D Scenes | `atoms_graph_scene_init_gl`, `atoms_graph_scene_render_stage`, `atoms_graph_scene_cleanup_gl` |
| `atoms_graph_metrics.c / .h` | Telemetry & Score Engine | `atoms_graph_metrics_init`, `atoms_graph_metrics_update_frame`, `atoms_graph_metrics_compute_score` |
| `atoms_graph_ui.c / .h` | 2D Panel UI Overlay | `atoms_graph_ui_render_panel`, `atoms_graph_ui_render_results` |

---

## 8. Verification & Build Integration

### `kernel/debug/test_gl_phase11.c` & `test_gl_phase11.h`
- **Purpose**: Phase 11 automated verification test suite (Tests A through R).
- **Who Calls It**: `kernel/kernel.c` (`run_phase11_gl_verification_suite`).
- **When to Read/Modify**: When adding new automated test cases or adjusting automated execution timeouts.

### `build.ps1`
- **Purpose**: Master PowerShell build script. Compiles all kernel C files with Clang/LLVM, assembles bootloaders, and links `build/kernel.bin`.
- **When to Read/Modify**: When adding new `.c` source files to the kernel build list.

### `tools/image_builder.c`
- **Purpose**: Native C host tool that packages `boot.bin`, `stage2.bin`, and `kernel.bin` into `build/OS.img` (FAT32 disk image).
