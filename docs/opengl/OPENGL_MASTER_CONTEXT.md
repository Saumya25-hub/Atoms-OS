# OPENGL MASTER CONTEXT — ATOMS OS GRAPHICS & BGL ARCHITECTURE

## 1. Architecture Overview & Technical Pipeline

The ATOMS OS graphics ecosystem is a software-rasterized, hardware-abstracted 3D rendering pipeline built directly inside the x86 32-bit kernel without third-party graphics dependencies.

### End-to-End Pipeline Map

```text
+-----------------------------------------------------------------------------------+
|                        ATOMS Native Application / App                             |
|          (e.g., ATOMS GRAPH 3D, DOOM, GL Demos, Desktop Shell Apps)              |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|               Horse Engine (Application Manager & Registry)                       |
|           Registers App IDs, Icon Assets, Title Strings, Launch Hooks             |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                  BWE V2.0 (BOSurface Window Engine) & Shell                       |
|       Manages Window Handles, Client Geometry, Event Dispatch, Z-Order            |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                 BGL Drawable (BGLDrawable Surface Binding)                        |
|       Allocates & Insets Color Buffer, Depth Buffer (float), Stencil (uint8)       |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|               BGL Context (BGLContext Active State Wrapper)                       |
|       Binds Target Drawable to Task Context, Coordinates bglSwapBuffers           |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                    Public OpenGL API Layer (gl.h / gl.c)                          |
|    glBegin/glEnd, glVertex3f, glColor3f, glTexCoord2f, glNormal3f, glFrustum,     |
|   glMatrixMode, glBindTexture, glGenFramebuffers, glDrawElements, etc.            |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                   OpenGL State Machine (GLContextState)                           |
|    ModelView/Projection Matrices, Lighting State, Material Props, Viewport,       |
|    Scissor Rect, Stencil State, Depth Function, Polygon Offset, Display Lists      |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                 Pipeline Assembly, Clipping & Geometry Setup                      |
|       Vertex Transformation (MVP Matrix), Sutherland-Hodgman Frustum Clipping,     |
|       Perspective Divide (Normalized Device Coordinates), Viewport Transformation  |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|             Rasterization & Fragment Processing Subsystems                        |
|    Barycentric Triangle Edge Functions, Sub-pixel Precision, Depth Testing (Z),   |
|    Stencil Masking & Ops, Bilinear/Trilinear Mipmap Texture Sampler, Alpha Blending|
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|           Offscreen Framebuffers / Render-to-Texture (FBO & RBO)                  |
|     Offscreen Color Texture Attachment, Depth/Stencil Renderbuffer Isolation,     |
|     glReadPixels, Two-Pass Texture Sampling, Feedback-Loop Protection             |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                  Target Surface / Window Canvas Output                            |
|       Color Buffer Blitting to BWE Canvas / Backing Pixel Buffer                  |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|              Compositor / BSPE Display HAL / Presentation Hardware                |
|      Damage Tracker, Present Queue, VBE Driver (VESA 1920x1080x32 bpp LFB)       |
+-----------------------------------------------------------------------------------+
```

---

## 2. Completed Phase Evolution (Phases 0–11)

### Phase 0–7: Foundations & Core 3D Rasterization
- **Phase 0 (CPU & Base Memory)**: Heap page allocation, aligned allocation primitives (`kmalloc_aligned`, `kfree_aligned`), SMP spinlock stability.
- **Phase 1 (BGL Surface Init)**: BGL drawable allocation, pitch alignment, 32-bpp ARGB target binding.
- **Phase 2 (Basic Geometry & Shading)**: Vertex transformation, flat/gouraud shading, MVP matrix stack (`glMatrixMode`, `glPushMatrix`, `glPopMatrix`).
- **Phase 3 (Perspective & Frustum Clipping)**: `glFrustum`, `gluPerspective` emulation, 6-plane frustum clipping in clip space ($w$ clipping), perspective-correct attribute interpolation ($1/w$, $u/w$, $v/w$, $r/w$, $g/w$, $b/w$).
- **Phase 4 (Depth Testing & Alpha Blending)**: 32-bit floating point depth buffer, `glDepthFunc`, `glDepthMask`, `glEnable(GL_BLEND)`, `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`.
- **Phase 5 (Fixed-Function Lighting)**: Ambient, diffuse, and specular lighting equations (`glLightfv`, `glMaterialfv`), face normal calculation (`glNormal3f`), transformed normal normalization.
- **Phase 6 (Texture Mapping Foundation)**: `glGenTextures`, `glBindTexture`, `glTexImage2D`, 2D nearest/bilinear texture sampling, wrap modes (`GL_REPEAT`, `GL_CLAMP`).
- **Phase 7 (Display Lists & Vertex Arrays)**: `glNewList`, `glEndList`, `glCallList`, display list compilation and execution replay, `glVertexPointer`, `glColorPointer`, `glTexCoordPointer`, `glDrawArrays`, `glDrawElements`.

### Phase 8: Mipmapping, TexSubImage & ReadPixels
- `glGenerateMipmap`, automatic downscaling pyramid creation down to 1x1.
- Mipmap filtering modes (`GL_NEAREST_MIPMAP_NEAREST`, `GL_LINEAR_MIPMAP_NEAREST`, `GL_LINEAR_MIPMAP_LINEAR` trilinear).
- `glTexSubImage2D` partial texture updates with boundary torture protection.
- `glReadPixels` frame buffer readback, `GL_PACK_ALIGNMENT` and `GL_UNPACK_ALIGNMENT` handling.

### Phase 9: Stencil Buffer, Polygon Offset & Face Culling
- 8-bit hardware stencil buffer (`uint8_t`), `glStencilFunc`, `glStencilOp`, `glStencilMask`.
- `glPolygonOffset` depth bias math for coplanar z-fighting elimination.
- `glCullFace` (`GL_FRONT`, `GL_BACK`) and `glFrontFace` (`GL_CW`, `GL_CCW`) winding order determination.

### Phase 10: Offscreen Rendering / Render-to-Texture (FBO & RBO)
- Framebuffer Objects (`glGenFramebuffers`, `glBindFramebuffer`, `glCheckFramebufferStatus`).
- Renderbuffer Objects (`glGenRenderbuffers`, `glBindRenderbuffer`, `glRenderbufferStorage`).
- Color texture attachments (`glFramebufferTexture2D`).
- Combined color, depth, and stencil FBO targets with two-pass Render-to-Texture (RTT).
- Feedback-loop safety prevention (disallowing bound texture sampling when bound as current render target).

### Phase 11: ATOMS GRAPH 3D Benchmark & Stress Application
- First native 3D graphics benchmark application for ATOMS OS.
- 10 progressive 3D workload stages (~5-minute workload, fast-stepping in test automation).
- Metrics engine tracking FPS (min/max/avg), frame time (ms), total triangles, draw calls, heap memory delta, resolution, stage progress.
- Formula-driven deterministic `ATOMS GRAPH SCORE`.
- Integrated side-panel telemetry overlay UI (`atoms_graph_ui.c`).

---

## 3. Key Architectural Decisions & Safeguards

1. **Window Engine & Pointer Safety (`bwe_window.c`)**:
   - BWE windows store application-specific data in `win->user_data`.
   - Desktop apps (e.g. ATOMS GRAPH 3D, DOOM) pass integer app IDs (`APP_ID_GRAPH_3D = 13`) in `win->user_data`.
   - **Crucial Guard**: When destroying windows in `BOS_DestroySurface` / `BWE_DestroyWindow`, the pointer sanity check `(uintptr_t)win->user_data > 4096` MUST be executed prior to `kfree(win->user_data)`. Failing to check this condition causes kernel heap crashes.

2. **Client Geometry Insets (`bgl_drawable.c`)**:
   - `bglCreateDrawableForWindow` calculates client bounds via `BWE_Geometry_CalculateClientBounds`.
   - A 640x480 decorated window yields a 640x440 client drawable area (insetting for the 40px titlebar).
   - Drawables allocate three aligned memory buffers:
     - `color_buffer` (`uint32_t *`, 32 bpp ARGB)
     - `depth_buffer` (`float *`, 32-bit float Z)
     - `stencil_buffer` (`uint8_t *`, 8-bit stencil)

3. **Software Rasterizer Sub-pixel Precision (`gl_rasterizer.c`)**:
   - Fixed-point 16.16 sub-pixel coordinates for triangle setup.
   - Perspective-correct interpolation using $1/w$ stepping across scanlines.
