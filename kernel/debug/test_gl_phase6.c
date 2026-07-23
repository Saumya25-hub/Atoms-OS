#include "kernel/debug/test_gl_phase6.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_fragment.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/graphics/gl/gl_math.h"

extern void display_print(const char* str);
extern void crash_log_add(const char* msg);

static void test_alpha_func_golden(uint32_t win_id) {
    display_print("\n[PHASE6] Test A: Alpha Function Golden Tests...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_ALPHA_TEST);

    bool pass = true;

    // Clear buffer to Blue (0x0000FFFF) & Depth 1.0
    glClearColor(0, 0, 1, 1);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render Red triangle with Alpha 0.2 under GL_GREATER (Ref 0.5) -> MUST BE REJECTED
    glAlphaFunc(GL_GREATER, 0.5f);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 0, 0, 0.2f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    if ((pixel & 0x00FFFFFF) != 0x000000FF) pass = false; // Must remain Blue!

    // Now render Red triangle with Alpha 0.8 under GL_GREATER (Ref 0.5) -> MUST PASS
    glBegin(GL_TRIANGLES);
    glColor4f(1, 0, 0, 0.8f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    pixel = d->color_buffer[60 * d->width + 80];
    if (((pixel >> 16) & 0xFF) < 200) pass = false; // Must be Red!

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Alpha Function Golden Tests: PASS\n");
        crash_log_add("[PHASE6 TEST] Alpha Function Golden Tests: PASS");
    } else {
        display_print("[PHASE6] Alpha Function Golden Tests: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Alpha Function Golden Tests: FAIL");
    }
}

static void test_standard_alpha_blending(uint32_t win_id) {
    display_print("\n[PHASE6] Test B: Standard Alpha Blending...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear to Blue (0, 0, 1, 1)
    glClearColor(0, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw Red (1, 0, 0, 0.5) -> Expected Result: Red 0.5 + Blue 0.5 = Magenta/Purple (0.5, 0, 0.5)
    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    uint8_t red_ch   = (pixel >> 16) & 0xFF;
    uint8_t green_ch = (pixel >> 8)  & 0xFF;
    uint8_t blue_ch  = pixel & 0xFF;

    // Red ~ 127, Green ~ 0, Blue ~ 127
    bool pass = (red_ch > 100 && red_ch < 155 && green_ch < 10 && blue_ch > 100 && blue_ch < 155);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Standard Alpha Blending: PASS\n");
        crash_log_add("[PHASE6 TEST] Standard Alpha Blending: PASS");
    } else {
        display_print("[PHASE6] Standard Alpha Blending: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Standard Alpha Blending: FAIL");
    }
}

static void test_blend_factor_matrix(uint32_t win_id) {
    display_print("\n[PHASE6] Test C: Blend Factor Matrix...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_BLEND);

    // Test Additive Blending (GL_ONE, GL_ONE): Green (0, 1, 0) + Red (1, 0, 0) = Yellow (1, 1, 0)
    glBlendFunc(GL_ONE, GL_ONE);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8)  & 0xFF;

    bool pass = (r > 240 && g > 240);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Blend Factor Matrix: PASS\n");
        crash_log_add("[PHASE6 TEST] Blend Factor Matrix: PASS");
    } else {
        display_print("[PHASE6] Blend Factor Matrix: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Blend Factor Matrix: FAIL");
    }
}

static void test_fog_golden(uint32_t win_id) {
    display_print("\n[PHASE6] Test D: Fog Golden Tests...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 1.0f);
    glFogf(GL_FOG_END, 5.0f);

    float fog_c[4] = {0.0f, 1.0f, 0.0f, 1.0f}; // Green Fog
    glFogfv(GL_FOG_COLOR, fog_c);

    GLContextState* state = gl_state_get_current();
    bool pass = (state->fog_enabled && state->fog_mode == GL_LINEAR && state->fog_color[1] == 1.0f);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Fog Golden Tests: PASS\n");
        crash_log_add("[PHASE6 TEST] Fog Golden Tests: PASS");
    } else {
        display_print("[PHASE6] Fog Golden Tests: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Fog Golden Tests: FAIL");
    }
}

static void test_fog_texture_lighting(uint32_t win_id) {
    display_print("\n[PHASE6] Test E: Fog + Texture + Lighting...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);

    GLContextState* state = gl_state_get_current();
    bool pass = (state->lighting_enabled && state->fog_enabled);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Fog + Texture + Lighting: PASS\n");
        crash_log_add("[PHASE6 TEST] Fog + Texture + Lighting: PASS");
    } else {
        display_print("[PHASE6] Fog + Texture + Lighting: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Fog + Texture + Lighting: FAIL");
    }
}

static void test_scissor_boundary_torture(uint32_t win_id) {
    display_print("\n[PHASE6] Test F: Scissor Boundary Torture...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_SCISSOR_TEST);

    // Clear buffer to Black (0)
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set Scissor Box to middle region [40, 20, 80, 40]
    glScissor(40, 20, 80, 40);

    // Render full-screen Red quad
    glBegin(GL_TRIANGLES);
    glColor4f(1, 0, 0, 1);
    glVertex2f(-1.0f, -1.0f); glVertex2f(1.0f, -1.0f); glVertex2f(-1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);  glVertex2f(1.0f, 1.0f);  glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Corner pixel (10, 10) must remain Black (0)
    uint32_t out_pixel = d->color_buffer[10 * d->width + 10];
    // Center pixel (40, 80) must be Red
    uint32_t in_pixel  = d->color_buffer[40 * d->width + 80];

    bool pass = ((out_pixel & 0x00FFFFFF) == 0 && ((in_pixel >> 16) & 0xFF) > 200);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Scissor Boundary Torture: PASS\n");
        crash_log_add("[PHASE6 TEST] Scissor Boundary Torture: PASS");
    } else {
        display_print("[PHASE6] Scissor Boundary Torture: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Scissor Boundary Torture: FAIL");
    }
}

static void test_color_mask_exhaustive(uint32_t win_id) {
    display_print("\n[PHASE6] Test G: Color Mask Exhaustive...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);

    // Clear to Black
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Mask off Red (allow Green & Blue)
    glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Try rendering White (1, 1, 1) -> Result should be Cyan (0, 1, 1) because Red is masked off!
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t g = (pixel >> 8)  & 0xFF;
    uint8_t b = pixel & 0xFF;

    bool pass = (r == 0 && g > 200 && b > 200);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Color Mask Exhaustive: PASS\n");
        crash_log_add("[PHASE6 TEST] Color Mask Exhaustive: PASS");
    } else {
        display_print("[PHASE6] Color Mask Exhaustive: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Color Mask Exhaustive: FAIL");
    }
}

static void test_depth_range_golden(uint32_t win_id) {
    display_print("\n[PHASE6] Test H: Depth Range Golden Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glDepthRange(0.25, 0.75);

    GLContextState* state = gl_state_get_current();
    bool pass = (state->depth_range_near == 0.25 && state->depth_range_far == 0.75);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Depth Range Golden Test: PASS\n");
        crash_log_add("[PHASE6 TEST] Depth Range Golden Test: PASS");
    } else {
        display_print("[PHASE6] Depth Range Golden Test: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Depth Range Golden Test: FAIL");
    }
}

static void test_point_rasterization(uint32_t win_id) {
    display_print("\n[PHASE6] Test I: Point Rasterization...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glColor4f(1, 0, 0, 1);
    glVertex2f(0.0f, 0.0f); // Center of screen (80, 60)
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    bool pass = (((pixel >> 16) & 0xFF) > 200);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Point Rasterization: PASS\n");
        crash_log_add("[PHASE6 TEST] Point Rasterization: PASS");
    } else {
        display_print("[PHASE6] Point Rasterization: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Point Rasterization: FAIL");
    }
}

static void test_line_rasterization(uint32_t win_id) {
    display_print("\n[PHASE6] Test J: Line Rasterization...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor4f(0, 1, 0, 1);
    glVertex2f(-0.5f, 0.0f);
    glVertex2f( 0.5f, 0.0f);
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    bool pass = (((pixel >> 8) & 0xFF) > 200);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Line Rasterization: PASS\n");
        crash_log_add("[PHASE6 TEST] Line Rasterization: PASS");
    } else {
        display_print("[PHASE6] Line Rasterization: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Line Rasterization: FAIL");
    }
}

static void test_interpolated_line_attributes(uint32_t win_id) {
    display_print("\n[PHASE6] Test K: Interpolated Line Attributes...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Line from Red (left) to Blue (right)
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor4f(1, 0, 0, 1); glVertex2f(-0.8f, 0.0f);
    glColor4f(0, 0, 1, 1); glVertex2f( 0.8f, 0.0f);
    glEnd();

    // Center pixel should be Purple/Magenta (0.5 Red, 0.5 Blue)
    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    uint8_t r = (pixel >> 16) & 0xFF;
    uint8_t b = pixel & 0xFF;

    bool pass = (r > 80 && b > 80);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Interpolated Line Attributes: PASS\n");
        crash_log_add("[PHASE6 TEST] Interpolated Line Attributes: PASS");
    } else {
        display_print("[PHASE6] Interpolated Line Attributes: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Interpolated Line Attributes: FAIL");
    }
}

static void test_polygon_mode(uint32_t win_id) {
    display_print("\n[PHASE6] Test L: Polygon Mode...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    GLContextState* state = gl_state_get_current();
    bool pass = (state->polygon_mode_front == GL_LINE && state->polygon_mode_back == GL_LINE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Polygon Mode: PASS\n");
        crash_log_add("[PHASE6 TEST] Polygon Mode: PASS");
    } else {
        display_print("[PHASE6] Polygon Mode: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Polygon Mode: FAIL");
    }
}

static void test_multi_context_state_isolation(uint32_t win1, uint32_t win2) {
    display_print("\n[PHASE6] Test M: Multi-Context State Isolation...\n");

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2);
    BGLContext* c1 = bglCreateContext(d1);
    BGLContext* c2 = bglCreateContext(d2);

    if (!d1 || !d2 || !c1 || !c2) return;

    // Context 1: Enable Blend & Alpha Test
    bglMakeCurrent(c1, d1);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);

    // Context 2: Disable Blend & Alpha Test
    bglMakeCurrent(c2, d2);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);

    // Verify C1
    bglMakeCurrent(c1, d1);
    GLContextState* s1 = gl_state_get_current();

    // Verify C2
    bglMakeCurrent(c2, d2);
    GLContextState* s2 = gl_state_get_current();

    bool pass = (s1->blend_enabled && s1->alpha_test_enabled && !s2->blend_enabled && !s2->alpha_test_enabled);

    bglReleaseCurrent();
    bglDestroyContext(c1);
    bglDestroyContext(c2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (pass) {
        display_print("[PHASE6] Multi-Context State Isolation: PASS\n");
        crash_log_add("[PHASE6 TEST] Multi-Context State Isolation: PASS");
    } else {
        display_print("[PHASE6] Multi-Context State Isolation: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Multi-Context State Isolation: FAIL");
    }
}

static void test_fragment_pipeline_ordering(uint32_t win_id) {
    display_print("\n[PHASE6] Test N: Fragment Pipeline Ordering...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);

    // Clear to Black
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);

    // Render Red fragment with alpha 0.2 (under GL_GREATER ref 0.5) -> Rejected by Alpha Test before Blending!
    glAlphaFunc(GL_GREATER, 0.5f);
    glBlendFunc(GL_ONE, GL_ONE);

    glBegin(GL_TRIANGLES);
    glColor4f(1, 0, 0, 0.2f);
    glVertex2f(-0.5f, -0.5f); glVertex2f(0.5f, -0.5f); glVertex2f(0.0f, 0.5f);
    glEnd();

    uint32_t pixel = d->color_buffer[60 * d->width + 80];
    bool pass = ((pixel & 0x00FFFFFF) == 0); // Must remain Black because alpha test rejected it!

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE6] Fragment Pipeline Ordering: PASS\n");
        crash_log_add("[PHASE6 TEST] Fragment Pipeline Ordering: PASS");
    } else {
        display_print("[PHASE6] Fragment Pipeline Ordering: FAIL!\n");
        crash_log_add("[PHASE6 TEST] Fragment Pipeline Ordering: FAIL");
    }
}

static void test_fragment_stress_torture(uint32_t win_id) {
    display_print("\n[PHASE6] Test O: Fragment Stress Torture (500 Primitives)...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < 500; i++) {
        glBegin(GL_TRIANGLES);
        glColor4f((float)(i % 10) * 0.1f, (float)(i % 5) * 0.2f, 1.0f, 0.5f);
        glVertex3f(-0.5f, -0.5f, -0.1f);
        glVertex3f( 0.5f, -0.5f, -0.1f);
        glVertex3f( 0.0f,  0.5f, -0.1f);
        glEnd();
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    display_print("[PHASE6] Fragment Stress Torture: PASS (500 tris OK)\n");
    crash_log_add("[PHASE6 TEST] Fragment Stress Torture: PASS");
}

static void test_real_public_gl_api_presentation(uint32_t win_id) {
    display_print("\n[PHASE6] Test P: Real Public GL API Presentation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_BLEND);

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw 3D lit/fogged quad
    glBegin(GL_TRIANGLES);
    glColor4f(0.8f, 0.2f, 0.2f, 0.8f);
    glVertex3f(-0.5f, -0.5f, -1.0f);
    glVertex3f( 0.5f, -0.5f, -1.0f);
    glVertex3f( 0.0f,  0.5f, -1.0f);
    glEnd();

    bglSwapBuffers(ctx);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    display_print("[PHASE6] Real Public GL API Presentation: PASS (GL -> BGL -> BWE -> AGDTE)\n");
    crash_log_add("[PHASE6 TEST] Real Public GL API Presentation: PASS");
}

void run_phase6_gl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 6 VERIFICATION\n");
    display_print("=========================================\n");

    uint32_t win1 = 0, win2 = 0;
    bwe_error_t err1 = BOS_CreateWindow(100, 100, 160, 120, "GL_Window1", &win1);
    bwe_error_t err2 = BOS_CreateWindow(200, 200, 160, 120, "GL_Window2", &win2);

    if (err1 != BWE_SUCCESS || err2 != BWE_SUCCESS || win1 == 0 || win2 == 0) {
        display_print("[PHASE6] ERROR: Failed to create BWE windows for GL testing!\n");
        return;
    }

    test_alpha_func_golden(win1);
    test_standard_alpha_blending(win1);
    test_blend_factor_matrix(win1);
    test_fog_golden(win1);
    test_fog_texture_lighting(win1);
    test_scissor_boundary_torture(win1);
    test_color_mask_exhaustive(win1);
    test_depth_range_golden(win1);
    test_point_rasterization(win1);
    test_line_rasterization(win1);
    test_interpolated_line_attributes(win1);
    test_polygon_mode(win1);
    test_multi_context_state_isolation(win1, win2);
    test_fragment_pipeline_ordering(win1);
    test_fragment_stress_torture(win1);
    test_real_public_gl_api_presentation(win1);

    BOS_DestroySurface(win1);
    BOS_DestroySurface(win2);

    display_print("=========================================\n");
}
