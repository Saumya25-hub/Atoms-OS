# OPENGL ARCHITECTURE FINAL SPECIFICATION
## ATOMS OS BGL Subsystem (Phases 1–14 Complete Architecture)

---

## 1. System Overview

ATOMS BGL is an OpenGL 1.1 / ES 1.1 compatible software-rasterized 3D graphics subsystem designed natively for ATOMS OS. It provides complete windowing integration with BOSurface Window Engine (BWE), deterministic memory management, 32-bit floating-point depth buffer evaluation, fixed-point barycentric rasterization, alpha blending, 2D texture mapping, and target-aware typography overlay rendering.

---

## 2. Subsystem Components & Responsibilities

### 2.1 BGL Core State Machine (`bgl.c` / `bgl_context.c`)
- Manages active BGL context (`BGLContext`) state including matrix mode, projection/modelview stacks, transformation matrices, clear values, depth function, scissor box, viewport parameters, and active drawables.
- Enforces strict null-pointer validation in `bglMakeCurrent()` and `bglSwapBuffers()`.

### 2.2 Geometry Pipeline (`bgl_geometry.c` / `bgl_matrix.c`)
- Multiplies object-space vertices by $M_{\text{ModelView}} \times M_{\text{Projection}}$ to calculate clip-space homogeneous coordinates $(x_c, y_c, z_c, w_c)$.
- Applies 3D frustum near/far plane clipping using the Sutherland-Hodgman algorithm before perspective division.
- Performs perspective division $(x_n, y_n, z_n) = (x_c/w_c, y_c/w_c, z_c/w_c)$ and maps NDC to window client-space coordinates $(x_w, y_w)$.

### 2.3 Sub-Pixel Barycentric Rasterizer (`bgl_raster.c`)
- Fixed-point sub-pixel barycentric rasterizer with linear interpolation of color $(r,g,b,a)$, depth $(z)$, and texture coordinates $(u,v)$.
- Validates each pixel against scissor bounds and 32-bit float Z-buffer.
- Dispatches texel sampling to `bgl_texture.c` and performs alpha blending before writing to `drawable->color_buffer`.

### 2.4 Texture & Image Engine (`bgl_texture.c`)
- Manages texture objects (`GL_TEXTURE_2D`), texture upload (`glTexImage2D`), wrapping (`GL_REPEAT`, `GL_CLAMP`), and fixed-point bilinear filtering.

### 2.5 Target-Aware Typography Overlay Engine (`bofont.c` / `boimage.c`)
- Provides `BOFont_DrawTextRoleTarget(target_fb, ...)` and `BOImage_DrawGlyphSpriteDirect(target_fb, ...)` for target-aware, zero-side-effect text rendering directly into offscreen surfaces (`d->color_buffer`) without global framebuffer context switching or batching delays.

---

## 3. Windowing & Composition Flow

1. Application launches via Horse Engine (`APP_ID_GRAPH_3D`).
2. Creates BWE Native Window (`BOS_CreateSurface`) and sets dedicated title (`BOS_SetText`).
3. Allocates BGL Context & Drawable (`bglCreateContext`, `bglCreateDrawable`).
4. Binds Context (`bglMakeCurrent`).
5. Renders 3D Scene (`bglBegin` .. `glEnd`).
6. Renders 2D Telemetry Panel (`atoms_graph_renderer_render_hud` into `client_fb`).
7. Swaps buffers (`bglSwapBuffers`), presenting `d->color_buffer` to BWE client surface.
8. BWE Compositor compositing window chrome, titlebar, and client surface onto display hardware.
