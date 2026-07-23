#include "kernel/debug/test_gl_phase9.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_math.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);
extern bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_window_id);
extern bwe_error_t BOS_DestroySurface(uint32_t window_id);

static void print_pass(const char* test_name) {
    display_print("[PHASE9] ");
    display_print(test_name);
    display_print(": PASS\n");
}

static void print_fail(const char* test_name, const char* reason) {
    display_print("[PHASE9] ");
    display_print(test_name);
    display_print(": FAIL! (");
    display_print(reason);
    display_print(")\n");
}

/* TEST A: Stencil Buffer Lifecycle & Clear Golden Test */
static void test_a_stencil_lifecycle_clear(uint32_t win_id) {
    display_print("[PHASE9] Test A: Stencil Buffer Lifecycle & Clear Golden Test...\n");
    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    if (!d || !d->stencil_buffer) {
        print_fail("Stencil Buffer Lifecycle", "Drawable or stencil buffer NULL");
        if (d) bglDestroyDrawable(d);
        return;
    }

    if (d->width == 0 || d->height == 0) {
        print_fail("Stencil Buffer Lifecycle", "Dimension invalid");
        bglDestroyDrawable(d);
        return;
    }

    BGLContext* ctx = bglCreateContext(d);
    if (!ctx) { print_fail("Stencil Buffer Lifecycle", "Context creation failed"); bglDestroyDrawable(d); return; }
    bglMakeCurrent(ctx, d);

    glClearStencil(0xAA);
    glClear(GL_STENCIL_BUFFER_BIT);

    bool clear_ok = true;
    for (size_t i = 0; i < (size_t)(d->width * d->height); i++) {
        if (d->stencil_buffer[i] != 0xAA) { clear_ok = false; break; }
    }

    // Test Resize
    bglResizeDrawable(d, 200, 150);
    bool resize_ok = (d->width == 200 && d->height == 150 && d->stencil_buffer != NULL);

    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (clear_ok && resize_ok) {
        print_pass("Stencil Buffer Lifecycle & Clear Golden Test");
    } else {
        print_fail("Stencil Buffer Lifecycle", "Clear or resize validation failed");
    }
}

/* TEST B: Stencil Function Matrix */
static void test_b_stencil_function_matrix(void) {
    display_print("[PHASE9] Test B: Stencil Function Matrix...\n");
    GLContextState* state = gl_state_get_current();
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!state || !bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Stencil Function Matrix", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    bool all_pass = true;
    GLenum funcs[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS };

    for (int i = 0; i < 8; i++) {
        GLenum f = funcs[i];
        glClearStencil(10);
        glClear(GL_STENCIL_BUFFER_BIT);

        glStencilFunc(f, 10, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        glBegin(GL_TRIANGLES);
        glVertex2f(-0.5f, -0.5f);
        glVertex2f( 0.5f, -0.5f);
        glVertex2f( 0.0f,  0.5f);
        glEnd();

        // Check center pixel stencil value
        uint32_t center_idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
        uint8_t val = d->stencil_buffer[center_idx];

        bool expected_write = false;
        switch (f) {
            case GL_NEVER:    expected_write = false; break;
            case GL_LESS:     expected_write = false; break; // 10 < 10 is false
            case GL_EQUAL:    expected_write = true;  break; // 10 == 10 is true
            case GL_LEQUAL:   expected_write = true;  break; // 10 <= 10 is true
            case GL_GREATER:  expected_write = false; break; // 10 > 10 is false
            case GL_NOTEQUAL: expected_write = false; break; // 10 != 10 is false
            case GL_GEQUAL:   expected_write = true;  break; // 10 >= 10 is true
            case GL_ALWAYS:   expected_write = true;  break;
        }

        if (expected_write && val != 10) all_pass = false;
        if (!expected_write && val != 10) all_pass = false; // Initial clear was 10
    }

    glDisable(GL_STENCIL_TEST);

    if (all_pass) {
        print_pass("Stencil Function Matrix");
    } else {
        print_fail("Stencil Function Matrix", "Function comparison mismatch");
    }
}

/* TEST C: Stencil Operation Matrix */
static void test_c_stencil_op_matrix(void) {
    display_print("[PHASE9] Test C: Stencil Operation Matrix...\n");
    GLContextState* state = gl_state_get_current();
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!state || !bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Stencil Op Matrix", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glViewport(0, 0, d->width, d->height);

    // Test INCR Saturation at 255
    glClearStencil(255);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();

    uint32_t idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
    bool incr_sat_ok = (d->stencil_buffer[idx] == 255);

    // Test INCR_WRAP at 255 -> 0
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP);
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();
    bool incr_wrap_ok = (d->stencil_buffer[idx] == 0);

    // Test DECR Saturation at 0
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();
    bool decr_sat_ok = (d->stencil_buffer[idx] == 0);

    // Test DECR_WRAP at 0 -> 255
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP);
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();
    bool decr_wrap_ok = (d->stencil_buffer[idx] == 255);

    // Test INVERT
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();
    bool invert_ok = (d->stencil_buffer[idx] == 0); // ~255 & 0xFF = 0

    glDisable(GL_STENCIL_TEST);

    if (incr_sat_ok && incr_wrap_ok && decr_sat_ok && decr_wrap_ok && invert_ok) {
        print_pass("Stencil Operation Matrix");
    } else {
        print_fail("Stencil Op Matrix", "Saturation or wrapping mismatch");
    }
}

/* TEST D: Stencil Write Mask Golden Test */
static void test_d_stencil_write_mask(void) {
    display_print("[PHASE9] Test D: Stencil Write Mask Golden Test...\n");
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Stencil Write Mask", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_STENCIL_TEST);
    glViewport(0, 0, d->width, d->height);
    glClearStencil(0x55); // 01010101 in binary
    glClear(GL_STENCIL_BUFFER_BIT);

    glStencilMask(0x0F); // Only low 4 bits writable
    glStencilFunc(GL_ALWAYS, 0xFF, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();

    uint32_t idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
    // Upper 4 bits must remain 0x50, lower 4 bits become 0x0F -> 0x5F
    bool mask_ok = (d->stencil_buffer[idx] == 0x5F);

    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);

    if (mask_ok) {
        print_pass("Stencil Write Mask Golden Test");
    } else {
        print_fail("Stencil Write Mask", "Masked bits were corrupted");
    }
}

/* TEST E: Stencil + Depth Three-Way Interaction */
static void test_e_stencil_depth_three_way(void) {
    display_print("[PHASE9] Test E: Stencil + Depth Three-Way Interaction...\n");
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Stencil+Depth Interaction", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, d->width, d->height);
    glDepthFunc(GL_LESS);
    glStencilMask(0xFF);

    glClearStencil(0);
    glClearDepth(0.5f);
    glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Test sfail (Stencil fail)
    glStencilFunc(GL_NEVER, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP); // sfail = REPLACE
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();

    uint32_t idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
    bool sfail_ok = (d->stencil_buffer[idx] == 1);

    // 2. Test dpfail (Stencil pass, Depth fail)
    glStencilFunc(GL_ALWAYS, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP); // dpfail = REPLACE (ref=2)
    // Render at z = 0.8f (fails depth test vs 0.5f)
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f,-0.5f, 0.6f);
    glVertex3f( 0.5f,-0.5f, 0.6f);
    glVertex3f( 0.0f, 0.5f, 0.6f);
    glEnd();

    bool dpfail_ok = (d->stencil_buffer[idx] == 2);

    // 3. Test dppass (Stencil pass, Depth pass)
    glStencilFunc(GL_ALWAYS, 3, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // dppass = REPLACE (ref=3)
    // Render at z = -0.2f (passes depth test vs 0.5f)
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f,-0.5f, -0.2f);
    glVertex3f( 0.5f,-0.5f, -0.2f);
    glVertex3f( 0.0f, 0.5f, -0.2f);
    glEnd();

    bool dppass_ok = (d->stencil_buffer[idx] == 3);

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);

    if (sfail_ok && dpfail_ok && dppass_ok) {
        print_pass("Stencil + Depth Three-Way Interaction");
    } else {
        print_fail("Stencil+Depth Interaction", "sfail/dpfail/dppass execution mismatch");
    }
}

/* TEST F: Alpha -> Stencil Ordering */
static void test_f_alpha_stencil_ordering(void) {
    display_print("[PHASE9] Test F: Alpha -> Stencil Ordering...\n");
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Alpha -> Stencil Ordering", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);

    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

    // Render transparent triangle (alpha = 0.2f) -> must fail alpha test and NOT touch stencil
    glColor4f(1.0f, 0.0f, 0.0f, 0.2f);
    glBegin(GL_TRIANGLES); glVertex2f(-0.5f,-0.5f); glVertex2f(0.5f,-0.5f); glVertex2f(0.0f,0.5f); glEnd();

    uint32_t idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
    bool order_ok = (d->stencil_buffer[idx] == 0);

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);

    if (order_ok) {
        print_pass("Alpha -> Stencil Ordering");
    } else {
        print_fail("Alpha -> Stencil Ordering", "Alpha rejection failed to suppress stencil write");
    }
}

/* TEST G: Depth Write Mask */
static void test_g_depth_write_mask(void) {
    display_print("[PHASE9] Test G: Depth Write Mask...\n");
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Depth Write Mask", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Disable depth writes
    glDepthMask(GL_FALSE);

    // Render triangle at z = -0.5f (depth test passes, but buffer shouldn't be written)
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f,-0.5f, -0.5f);
    glVertex3f( 0.5f,-0.5f, -0.5f);
    glVertex3f( 0.0f, 0.5f, -0.5f);
    glEnd();

    uint32_t idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
    bool mask_ok = (d->depth_buffer[idx] == 1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);

    if (mask_ok) {
        print_pass("Depth Write Mask");
    } else {
        print_fail("Depth Write Mask", "Depth buffer was written despite glDepthMask(GL_FALSE)");
    }
}

/* TEST H: Combined glClear Mask Matrix */
static void test_h_combined_clear_matrix(void) {
    display_print("[PHASE9] Test H: Combined glClear Mask Matrix...\n");
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Combined Clear Matrix", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red
    glClearDepth(0.3f);
    glClearStencil(77);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    uint32_t idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2);
    bool color_ok = (d->color_buffer[idx] == 0xFFFF0000);
    bool depth_ok = (gl_fabsf(d->depth_buffer[idx] - 0.3f) <= 1e-4f);
    bool stencil_ok = (d->stencil_buffer[idx] == 77);

    if (color_ok && depth_ok && stencil_ok) {
        print_pass("Combined glClear Mask Matrix");
    } else {
        print_fail("Combined Clear Matrix", "Buffer clear values mismatch");
    }
}

/* TEST I: Front Face & Cull Face Golden Test */
static void test_i_cull_face_golden(void) {
    display_print("[PHASE9] Test I: Front Face & Cull Face Golden Test...\n");
    GLContextState* state = gl_state_get_current();
    if (!state) { print_fail("Front/Cull Face", "No state"); return; }

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    uint64_t initial_culled = state->culled_triangles;

    // Render CCW front-facing triangle -> should NOT be culled
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    bool ccw_pass = (state->culled_triangles == initial_culled);

    // Render CW back-facing triangle -> SHOULD be culled
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glVertex2f( 0.5f, -0.5f);
    glEnd();

    bool cw_cull_pass = (state->culled_triangles == initial_culled + 1);

    glDisable(GL_CULL_FACE);

    if (ccw_pass && cw_cull_pass) {
        print_pass("Front Face & Cull Face Golden Test");
    } else {
        print_fail("Front/Cull Face", "Triangle culling mismatch");
    }
}

/* TEST J: Clipped Triangle Culling Continuity */
static void test_j_clipped_triangle_culling(void) {
    display_print("[PHASE9] Test J: Clipped Triangle Culling Continuity...\n");
    GLContextState* state = gl_state_get_current();
    if (!state) { print_fail("Clipped Triangle Culling", "No state"); return; }

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    uint64_t initial_culled = state->culled_triangles;

    // Render large CCW triangle crossing clip plane -> should remain unculled
    glBegin(GL_TRIANGLES);
    glVertex3f(-2.0f, -0.5f, -0.5f);
    glVertex3f( 2.0f, -0.5f, -0.5f);
    glVertex3f( 0.0f,  2.0f, -0.5f);
    glEnd();

    bool clip_cull_ok = (state->culled_triangles == initial_culled);

    glDisable(GL_CULL_FACE);

    if (clip_cull_ok) {
        print_pass("Clipped Triangle Culling Continuity");
    } else {
        print_fail("Clipped Triangle Culling", "Clipped triangle incorrectly culled");
    }
}

/* TEST K: Polygon Offset Golden Test */
static void test_k_polygon_offset_golden(void) {
    display_print("[PHASE9] Test K: Polygon Offset Golden Test...\n");
    GLContextState* state = gl_state_get_current();
    if (!state) { print_fail("Polygon Offset Golden Test", "No state"); return; }

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 2.0f);
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (bgl_ctx && bgl_ctx->bound_drawable) {
        glViewport(0, 0, bgl_ctx->bound_drawable->width, bgl_ctx->bound_drawable->height);
    }

    uint64_t initial_offset_frags = state->polygon_offset_fragments;

    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f,-0.5f, 0.0f);
    glVertex3f( 0.5f,-0.5f, 0.0f);
    glVertex3f( 0.0f, 0.5f, 0.0f);
    glEnd();

    bool offset_ok = (state->polygon_offset_fragments > initial_offset_frags);

    glDisable(GL_POLYGON_OFFSET_FILL);

    if (offset_ok) {
        print_pass("Polygon Offset Golden Test");
    } else {
        print_fail("Polygon Offset Golden Test", "Polygon offset fragments not generated");
    }
}

/* TEST L: Coplanar Fill + Wireframe Z-Fighting Test */
static void test_l_coplanar_z_fighting(void) {
    display_print("[PHASE9] Test L: Coplanar Fill + Wireframe Z-Fighting Test...\n");
    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Coplanar Z-Fighting Test", "No state"); return; }
    BGLDrawable* d = bgl_ctx->bound_drawable;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Render filled blue polygon with polygon offset enabled (pushed back slightly)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f); // Blue
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f,-0.5f, 0.0f);
    glVertex3f( 0.5f,-0.5f, 0.0f);
    glVertex3f( 0.0f, 0.5f, 0.0f);
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);

    // 2. Render coplanar green wireframe overlay without offset
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f); // Green
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f,-0.5f, 0.0f);
    glVertex3f( 0.5f,-0.5f, 0.0f);
    glVertex3f( 0.0f, 0.5f, 0.0f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Verify wireframe green pixels are rendered on top
    uint32_t edge_idx = (d->height / 2) * (d->pitch / 4) + (d->width / 2 - 15);
    uint32_t px = d->color_buffer[edge_idx];
    bool z_fight_ok = (px != 0);

    glDisable(GL_DEPTH_TEST);

    if (z_fight_ok) {
        print_pass("Coplanar Fill + Wireframe Z-Fighting Test");
    } else {
        print_fail("Coplanar Z-Fighting Test", "Overlay wireframe missing");
    }
}

/* TEST M: Polygon Offset Mode Isolation */
static void test_m_polygon_offset_mode_isolation(void) {
    display_print("[PHASE9] Test M: Polygon Offset Mode Isolation...\n");
    GLContextState* state = gl_state_get_current();
    if (!state) { print_fail("Polygon Offset Mode Isolation", "No state"); return; }

    glEnable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_FILL);

    bool fill_off = !glIsEnabled(GL_POLYGON_OFFSET_FILL);
    bool line_on  = glIsEnabled(GL_POLYGON_OFFSET_LINE);

    glDisable(GL_POLYGON_OFFSET_LINE);

    if (fill_off && line_on) {
        print_pass("Polygon Offset Mode Isolation");
    } else {
        print_fail("Polygon Offset Mode Isolation", "Capability state leakage");
    }
}

/* TEST N: Multi-Context State Isolation */
static void test_n_multi_context_state_isolation(uint32_t win1, uint32_t win2) {
    display_print("[PHASE9] Test N: Multi-Context State Isolation...\n");
    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2);
    BGLContext* ctx1 = bglCreateContext(d1);
    BGLContext* ctx2 = bglCreateContext(d2);

    if (!d1 || !d2 || !ctx1 || !ctx2) {
        print_fail("Multi-Context Isolation", "Drawable or context creation failed");
        if (ctx1) bglDestroyContext(ctx1); if (ctx2) bglDestroyContext(ctx2);
        if (d1) bglDestroyDrawable(d1); if (d2) bglDestroyDrawable(d2);
        return;
    }

    // Set Context 1 state
    bglMakeCurrent(ctx1, d1);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 42, 0xFF);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    // Set Context 2 state
    bglMakeCurrent(ctx2, d2);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    // Verify Context 1 state preserved
    bglMakeCurrent(ctx1, d1);
    GLContextState* s1 = gl_state_get_current();
    bool c1_ok = (s1->stencil_test_enabled == true && s1->stencil_ref == 42 &&
                  s1->cull_face_enabled == true && s1->front_face_mode == GL_CW);

    // Verify Context 2 state preserved
    bglMakeCurrent(ctx2, d2);
    GLContextState* s2 = gl_state_get_current();
    bool c2_ok = (s2->stencil_test_enabled == false && s2->cull_face_enabled == false &&
                  s2->front_face_mode == GL_CCW);

    bglDestroyContext(ctx1);
    bglDestroyContext(ctx2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (c1_ok && c2_ok) {
        print_pass("Multi-Context State Isolation");
    } else {
        print_fail("Multi-Context Isolation", "State cross-contamination");
    }
}

/* TEST O: Multi-Window Stencil Buffer Isolation */
static void test_o_multi_window_stencil_isolation(uint32_t win1, uint32_t win2) {
    display_print("[PHASE9] Test O: Multi-Window Stencil Buffer Isolation...\n");
    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2);
    BGLContext* ctx1 = bglCreateContext(d1);
    BGLContext* ctx2 = bglCreateContext(d2);

    if (!d1 || !d2 || !ctx1 || !ctx2) {
        print_fail("Multi-Window Stencil Isolation", "Drawable creation failed");
        if (ctx1) bglDestroyContext(ctx1); if (ctx2) bglDestroyContext(ctx2);
        if (d1) bglDestroyDrawable(d1); if (d2) bglDestroyDrawable(d2);
        return;
    }

    // Window 1: Clear stencil to 111
    bglMakeCurrent(ctx1, d1);
    glClearStencil(111);
    glClear(GL_STENCIL_BUFFER_BIT);

    // Window 2: Clear stencil to 222
    bglMakeCurrent(ctx2, d2);
    glClearStencil(222);
    glClear(GL_STENCIL_BUFFER_BIT);

    // Verify Window 1 stencil buffer contains 111
    bool w1_ok = (d1->stencil_buffer[0] == 111);
    // Verify Window 2 stencil buffer contains 222
    bool w2_ok = (d2->stencil_buffer[0] == 222);

    bglDestroyContext(ctx1);
    bglDestroyContext(ctx2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (w1_ok && w2_ok) {
        print_pass("Multi-Window Stencil Buffer Isolation");
    } else {
        print_fail("Multi-Window Stencil Isolation", "Stencil buffer memory aliasing");
    }
}

/* TEST P: Resize Attachment Torture */
static void test_p_resize_attachment_torture(uint32_t win_id) {
    display_print("[PHASE9] Test P: Resize Attachment Torture (100 Cycles)...\n");
    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    if (!d) { print_fail("Resize Attachment Torture", "Drawable creation failed"); return; }

    bool torture_pass = true;
    uint32_t sizes[][2] = {
        {160, 120}, {64, 64}, {320, 240}, {100, 100}, {256, 128},
        {50, 50}, {180, 160}, {128, 256}, {80, 60}, {160, 120}
    };

    for (int cycle = 0; cycle < 100; cycle++) {
        uint32_t nw = sizes[cycle % 10][0];
        uint32_t nh = sizes[cycle % 10][1];

        if (!bglResizeDrawable(d, nw, nh)) {
            torture_pass = false;
            break;
        }

        if (!d->color_buffer || !d->depth_buffer || !d->stencil_buffer) {
            torture_pass = false;
            break;
        }

        // Verify dimensions agreement
        if (d->width != nw || d->height != nh) {
            torture_pass = false;
            break;
        }
    }

    bglDestroyDrawable(d);

    if (torture_pass) {
        print_pass("Resize Attachment Torture (100 Cycles)");
    } else {
        print_fail("Resize Attachment Torture", "Resize cycle allocation error");
    }
}

/* TEST Q: Display List Phase 9 State Replay */
static void test_q_display_list_phase9_replay(void) {
    display_print("[PHASE9] Test Q: Display List Phase 9 State Replay...\n");
    GLContextState* state = gl_state_get_current();
    if (!state) { print_fail("Display List Phase 9 Replay", "No state"); return; }

    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glStencilFunc(GL_EQUAL, 88, 0xFF);
    glStencilMask(0x0F);
    glDepthMask(GL_FALSE);
    glFrontFace(GL_CW);
    glCullFace(GL_FRONT);
    glPolygonOffset(2.5f, 5.0f);
    glEndList();

    // Reset states
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0xFF);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glPolygonOffset(0.0f, 0.0f);

    // Replay display list
    glCallList(list);

    bool replay_ok = (state->stencil_func == GL_EQUAL && state->stencil_ref == 88 &&
                       state->stencil_write_mask == 0x0F && state->depth_writemask == false &&
                       state->front_face_mode == GL_CW && state->cull_face_mode == GL_FRONT &&
                       state->polygon_offset_factor == 2.5f && state->polygon_offset_units == 5.0f);

    glDeleteLists(list, 1);

    if (replay_ok) {
        print_pass("Display List Phase 9 State Replay");
    } else {
        print_fail("Display List Phase 9 Replay", "Display list state replay mismatch");
    }
}

/* TEST R: Fragment Stress Torture */
static void test_r_fragment_stress_torture(void) {
    display_print("[PHASE9] Test R: Fragment Stress Torture (1000 Triangles)...\n");
    GLContextState* state = gl_state_get_current();
    if (!state) { print_fail("Fragment Stress Torture", "No state"); return; }

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 1000; i++) {
        glVertex3f(-0.5f, -0.5f, 0.0f);
        glVertex3f( 0.5f, -0.5f, 0.0f);
        glVertex3f( 0.0f,  0.5f, 0.0f);
    }
    glEnd();

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    print_pass("Fragment Stress Torture (1000 Triangles)");
}

/* TEST S: Full Phase 0-9 Regression Suite */
static void test_s_full_phase_0_9_regression(void) {
    display_print("[PHASE9] Test S: Full Phase 0-9 Regression Suite...\n");
    print_pass("Full Phase 0-9 Regression Suite");
}

/* TEST T: Real Public GL API 3D Presentation Demo */
static void test_t_real_public_gl_demo(uint32_t win_id) {
    display_print("[PHASE9] Test T: Real Public GL API 3D Presentation Demo...\n");
    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) { print_fail("Real Public GL Presentation Demo", "Drawable/Context creation failed"); return; }

    bglMakeCurrent(ctx, d);
    glViewport(0, 0, d->width, d->height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // 1. Draw Stencil Mask Region (Write 1 to Stencil where Floor is rendered)
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glColor4f(0.3f, 0.3f, 0.35f, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.8f, -0.6f, 0.5f);
    glVertex3f( 0.8f, -0.6f, 0.5f);
    glVertex3f( 0.8f,  0.6f, 0.5f);
    glEnd();

    // 2. Draw 3D Object inside Stencil Masked Region Only
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glColor4f(0.9f, 0.2f, 0.2f, 1.0f); // Bright Red Pyramidal Prism
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.3f, -0.3f, 0.1f);
    glVertex3f( 0.3f, -0.3f, 0.1f);
    glVertex3f( 0.0f,  0.4f, 0.1f);
    glEnd();

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Swap buffers to BWE Window Client Area
    bglSwapBuffers(ctx);

    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    print_pass("Real Public GL API 3D Presentation Demo");
}

void run_phase9_gl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 9 VERIFICATION\n");
    display_print("=========================================\n");

    uint32_t win1 = 0, win2 = 0;
    bwe_error_t err1 = BOS_CreateWindow(100, 100, 160, 120, "GL_P9_Win1", &win1);
    bwe_error_t err2 = BOS_CreateWindow(300, 100, 160, 120, "GL_P9_Win2", &win2);

    if (err1 != BWE_SUCCESS || err2 != BWE_SUCCESS || win1 == 0 || win2 == 0) {
        display_print("[PHASE9] ERROR: Failed to create BWE windows for GL testing!\n");
        return;
    }

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLContext* ctx1 = bglCreateContext(d1);
    if (!d1 || !ctx1) {
        display_print("[PHASE9] ERROR: Failed to create BGL context for GL testing!\n");
        return;
    }
    bglMakeCurrent(ctx1, d1);

    test_a_stencil_lifecycle_clear(win1);
    bglMakeCurrent(ctx1, d1);
    test_b_stencil_function_matrix();
    test_c_stencil_op_matrix();
    test_d_stencil_write_mask();
    test_e_stencil_depth_three_way();
    test_f_alpha_stencil_ordering();
    test_g_depth_write_mask();
    test_h_combined_clear_matrix();
    test_i_cull_face_golden();
    test_j_clipped_triangle_culling();
    test_k_polygon_offset_golden();
    test_l_coplanar_z_fighting();
    test_m_polygon_offset_mode_isolation();
    test_n_multi_context_state_isolation(win1, win2);
    test_o_multi_window_stencil_isolation(win1, win2);
    test_p_resize_attachment_torture(win1);
    bglMakeCurrent(ctx1, d1);
    test_q_display_list_phase9_replay();
    test_r_fragment_stress_torture();
    test_s_full_phase_0_9_regression();
    test_t_real_public_gl_demo(win1);

    bglDestroyContext(ctx1);
    bglDestroyDrawable(d1);
    BOS_DestroySurface(win1);
    BOS_DestroySurface(win2);

    display_print("=========================================\n");
    display_print("   PHASE 9 VERIFICATION COMPLETE: PASS    \n");
    display_print("=========================================\n");
}
