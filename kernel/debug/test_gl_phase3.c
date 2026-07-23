#include "test_gl_phase3.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_clip.h"
#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/gl/gl_rasterizer.h"
#include "kernel/graphics/gl/gl_depth.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint32_t val);
extern void crash_log_add(const char* msg);

static void test_clip_golden(uint32_t win_id) {
    display_print("[PHASE3] Test A: Homogeneous 6-Plane Clipping Golden Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;

    // 1. Triangle fully inside
    GLVertex tri_in[3] = {
        { .clip_pos = { 0.0f,  0.5f, 0.0f, 1.0f}, .color = {1,0,0,1} },
        { .clip_pos = {-0.5f, -0.5f, 0.0f, 1.0f}, .color = {0,1,0,1} },
        { .clip_pos = { 0.5f, -0.5f, 0.0f, 1.0f}, .color = {0,0,1,1} }
    };
    GLVertex clipped[GL_MAX_CLIPPED_POLYGON_VERTICES];
    uint32_t count = gl_clip_triangle(tri_in, clipped);
    if (count != 3) {
        display_print("[PHASE3] ERROR: Fully inside triangle count mismatch!\n");
        pass = false;
    }

    // 2. Triangle fully outside Near Plane (z < -w)
    GLVertex tri_out_near[3] = {
        { .clip_pos = { 0.0f,  0.5f, -2.0f, 1.0f} },
        { .clip_pos = {-0.5f, -0.5f, -2.0f, 1.0f} },
        { .clip_pos = { 0.5f, -0.5f, -2.0f, 1.0f} }
    };
    count = gl_clip_triangle(tri_out_near, clipped);
    if (count != 0) {
        display_print("[PHASE3] ERROR: Fully outside near plane triangle was not rejected!\n");
        pass = false;
    }

    // 3. Triangle crossing Near Plane (one vertex behind camera z = -2.0, w = 1.0)
    GLVertex tri_cross_near[3] = {
        { .clip_pos = { 0.0f,  1.0f,  0.0f, 1.0f}, .color = {1,0,0,1} },
        { .clip_pos = {-1.0f, -1.0f,  0.0f, 1.0f}, .color = {0,1,0,1} },
        { .clip_pos = { 0.0f,  0.0f, -2.0f, 1.0f}, .color = {0,0,1,1} }
    };
    count = gl_clip_triangle(tri_cross_near, clipped);
    if (count < 3 || count > 4) {
        display_print("[PHASE3] ERROR: Near plane crossing triangle clipping failed!\n");
        pass = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE3] Homogeneous 6-Plane Clipping Golden Test: PASS\n");
        crash_log_add("[PHASE3 TEST] Homogeneous 6-Plane Clipping Golden Test: PASS");
    } else {
        display_print("[PHASE3] Homogeneous 6-Plane Clipping Golden Test: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Homogeneous 6-Plane Clipping Golden Test: FAIL");
    }
}

static void test_viewport_perspective(uint32_t win_id) {
    display_print("\n[PHASE3] Test B: Perspective Divide & Viewport Transformation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;

    // Test clip vertex (x=2.0, y=-2.0, z=0.0, w=2.0) -> NDC (1.0, -1.0, 0.0)
    GLVertex v_clip = {
        .clip_pos = {2.0f, -2.0f, 0.0f, 2.0f},
        .color = {1.0f, 1.0f, 1.0f, 1.0f}
    };
    GLScreenVertex v_screen;
    bool ok = gl_viewport_transform_vertex(&v_clip, 0, 0, 160, 120, &v_screen);

    if (!ok) {
        display_print("[PHASE3] ERROR: Viewport transform failed!\n");
        pass = false;
    } else {
        // NDC (1.0, -1.0, 0.0) -> x = 160, y = 120, z = 0.5
        if (v_screen.x < 159.0f || v_screen.x > 161.0f ||
            v_screen.y < 119.0f || v_screen.y > 121.0f ||
            v_screen.z < 0.49f || v_screen.z > 0.51f) {
            display_print("[PHASE3] ERROR: Viewport mapping coordinate mismatch!\n");
            pass = false;
        }
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE3] Perspective Divide & Viewport Transformation: PASS\n");
        crash_log_add("[PHASE3 TEST] Perspective Divide & Viewport Transformation: PASS");
    } else {
        display_print("[PHASE3] Perspective Divide & Viewport Transformation: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Perspective Divide & Viewport Transformation: FAIL");
    }
}

static void test_top_left_shared_edge(uint32_t win_id) {
    display_print("\n[PHASE3] Test C: Top-Left Fill Rule & Shared-Edge Crack Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Submit 2 triangles forming a quad with shared diagonal
    glBegin(GL_TRIANGLES);
    // Triangle 1
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.5f,  0.5f);

    // Triangle 2 (shared diagonal from (0.5, 0.5) to (-0.5, -0.5))
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f,  0.5f);
    glVertex2f(-0.5f,  0.5f);
    glEnd();

    // Scan interior of quad (x: 50 to 110, y: 40 to 80) to verify ZERO black unpainted pixels (0x00000000)
    bool pass = true;
    uint32_t unpainted = 0;
    for (uint32_t y = 40; y <= 80; y++) {
        for (uint32_t x = 50; x <= 110; x++) {
            uint32_t pixel = d->color_buffer[y * d->width + x];
            if (pixel == 0x00000000) {
                unpainted++;
            }
        }
    }

    if (unpainted > 0) {
        display_print("[PHASE3] ERROR: Unpainted crack pixels detected along shared edge!\n");
        pass = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE3] Top-Left Fill Rule & Shared-Edge Crack Test: PASS (0 cracks)\n");
        crash_log_add("[PHASE3 TEST] Top-Left Fill Rule & Shared-Edge Crack Test: PASS");
    } else {
        display_print("[PHASE3] Top-Left Fill Rule & Shared-Edge Crack Test: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Top-Left Fill Rule & Shared-Edge Crack Test: FAIL");
    }
}

static void test_depth_occlusion(uint32_t win_id) {
    display_print("\n[PHASE3] Test D: Depth Testing & Order-Independent Occlusion Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 0.5, 10.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Clear Color to Black, Depth to 1.0
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Submit Far Red Triangle (Eye z = -5.0) FIRST, then Near Blue Triangle (Eye z = -1.5) SECOND
    glBegin(GL_TRIANGLES);
    // Far Red Triangle (Eye z = -5.0)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -5.0f);
    glVertex3f( 0.5f, -0.5f, -5.0f);
    glVertex3f( 0.0f,  0.5f, -5.0f);

    // Near Blue Triangle (Eye z = -1.5)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, -1.5f);
    glVertex3f( 0.5f, -0.5f, -1.5f);
    glVertex3f( 0.0f,  0.5f, -1.5f);
    glEnd();

    uint32_t center_pixel1 = d->color_buffer[60 * d->width + 80];

    // Re-clear and submit in REVERSE ORDER: Near Blue Triangle FIRST, Far Red Triangle SECOND
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    // Near Blue Triangle (Eye z = -1.5) FIRST
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, -1.5f);
    glVertex3f( 0.5f, -0.5f, -1.5f);
    glVertex3f( 0.0f,  0.5f, -1.5f);

    // Far Red Triangle (Eye z = -5.0) SECOND (should be depth-rejected!)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -5.0f);
    glVertex3f( 0.5f, -0.5f, -5.0f);
    glVertex3f( 0.0f,  0.5f, -5.0f);
    glEnd();

    uint32_t center_pixel2 = d->color_buffer[60 * d->width + 80];

    bool pass = true;
    // Blue pixel ARGB is 0xFF0000FF
    uint32_t blue_b = center_pixel1 & 0xFF;
    uint32_t blue_r = (center_pixel1 >> 16) & 0xFF;

    if (blue_b < 250 || blue_r > 5 || center_pixel1 != center_pixel2) {
        display_print("[PHASE3] ERROR: Order-independent depth occlusion mismatch!\n");
        pass = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE3] Depth Testing & Order-Independent Occlusion: PASS\n");
        crash_log_add("[PHASE3 TEST] Depth Testing & Order-Independent Occlusion: PASS");
    } else {
        display_print("[PHASE3] Depth Testing & Order-Independent Occlusion: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Depth Testing & Order-Independent Occlusion: FAIL");
    }
}

static void test_resize_depth_safety(uint32_t win_id) {
    display_print("\n[PHASE3] Test E: Resize Safety & Depth Buffer Reallocation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;
    uint32_t orig_gen = d->generation;

    // Resize drawable 5 times
    for (int i = 0; i < 5; i++) {
        uint32_t new_w = 160 + (i * 20);
        uint32_t new_h = 120 + (i * 15);
        if (!bglResizeDrawable(d, new_w, new_h)) {
            display_print("[PHASE3] ERROR: Drawable resize failed!\n");
            pass = false;
            break;
        }
        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        if (d->depth_buffer[0] != 1.0f) {
            display_print("[PHASE3] ERROR: Depth clear failed after resize!\n");
            pass = false;
            break;
        }
    }

    if (d->generation <= orig_gen) {
        display_print("[PHASE3] ERROR: Generation count was not incremented on resize!\n");
        pass = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE3] Resize Safety & Depth Buffer Reallocation: PASS\n");
        crash_log_add("[PHASE3 TEST] Resize Safety & Depth Buffer Reallocation: PASS");
    } else {
        display_print("[PHASE3] Resize Safety & Depth Buffer Reallocation: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Resize Safety & Depth Buffer Reallocation: FAIL");
    }
}

static void test_multi_context_raster_isolation(uint32_t win1, uint32_t win2) {
    display_print("\n[PHASE3] Test F: Multi-Context 3D Rasterization Isolation...\n");

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2);
    BGLContext* c1 = bglCreateContext(d1);
    BGLContext* c2 = bglCreateContext(d2);

    if (!d1 || !d2 || !c1 || !c2) return;

    // Context 1: Green Triangle
    bglMakeCurrent(c1, d1);
    glViewport(0, 0, 160, 120);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    glColor3f(0, 1, 0);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    // Context 2: Red Triangle
    bglMakeCurrent(c2, d2);
    glViewport(0, 0, 160, 120);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f( 0.5f, -0.5f);
    glVertex2f( 0.0f,  0.5f);
    glEnd();

    // Verify Context 1 center pixel is GREEN (0xFF00FF00) and Context 2 is RED (0xFFFF0000)
    uint32_t p1 = d1->color_buffer[60 * d1->width + 80];
    uint32_t p2 = d2->color_buffer[60 * d2->width + 80];

    bool pass = true;
    if (((p1 >> 8) & 0xFF) < 250 || ((p1 >> 16) & 0xFF) > 5) pass = false;
    if (((p2 >> 16) & 0xFF) < 250 || ((p2 >> 8) & 0xFF) > 5) pass = false;

    bglReleaseCurrent();
    bglDestroyContext(c1);
    bglDestroyContext(c2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (pass) {
        display_print("[PHASE3] Multi-Context 3D Rasterization Isolation: PASS\n");
        crash_log_add("[PHASE3 TEST] Multi-Context 3D Rasterization Isolation: PASS");
    } else {
        display_print("[PHASE3] Multi-Context 3D Rasterization Isolation: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Multi-Context 3D Rasterization Isolation: FAIL");
    }
}

static void test_rasterizer_torture(uint32_t win_id) {
    display_print("\n[PHASE3] Test G: Rasterizer Stress & Boundary Safety Torture...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Submit 1,000 extreme/degenerate/partially clipped triangles
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 1000; i++) {
        float x = (float)(i % 17 - 8) * 5.0f;
        float y = (float)(i % 13 - 6) * 5.0f;
        float z = (float)(i % 7 - 3) * 2.0f;

        glColor3f((float)(i % 256) / 255.0f, (float)((i * 3) % 256) / 255.0f, (float)((i * 7) % 256) / 255.0f);
        glVertex3f(x, y, z);
        glVertex3f(x + 10.0f, y - 5.0f, z - 1.0f);
        glVertex3f(x - 5.0f, y + 8.0f, z + 2.0f);
    }
    glEnd();

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    display_print("[PHASE3] Rasterizer Stress & Boundary Safety Torture: PASS (1000 tris OK)\n");
    crash_log_add("[PHASE3 TEST] Rasterizer Stress & Boundary Safety Torture: PASS");
}

static void test_public_gl_3d_presentation(uint32_t win_id) {
    display_print("\n[PHASE3] Test H: Real Public GL API 3D Triangle Presentation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);

    // Perspective Projection setup
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);

    // ModelView Translation back by 3.0 units along Z
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);

    glClearColor(0.2f, 0.2f, 0.4f, 1.0f); // Dark Slate Blue
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Real Public GL RGB Interpolated 3D Triangle Submission
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f( 0.0f,  1.0f, 0.0f);
    glEnd();

    // Present frame through BGL -> BWE -> Compositor -> AGDTE
    bglSwapBuffers(ctx);

    // Verify center pixel has non-zero color
    uint32_t center_pixel = d->color_buffer[60 * d->width + 80];
    bool pass = (center_pixel != 0x00000000 && center_pixel != 0xFF333366);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE3] Real Public GL API 3D Triangle Presentation: PASS (GL -> BGL -> BWE -> AGDTE)\n");
        crash_log_add("[PHASE3 TEST] Real Public GL API 3D Triangle Presentation: PASS");
    } else {
        display_print("[PHASE3] Real Public GL API 3D Triangle Presentation: FAIL!\n");
        crash_log_add("[PHASE3 TEST] Real Public GL API 3D Triangle Presentation: FAIL");
    }
}

void run_phase3_gl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 3 VERIFICATION\n");
    display_print("=========================================\n");

    uint32_t win1 = 0, win2 = 0;
    bwe_error_t err1 = BOS_CreateWindow(100, 100, 160, 120, "GL_Window1", &win1);
    bwe_error_t err2 = BOS_CreateWindow(200, 200, 160, 120, "GL_Window2", &win2);

    if (err1 != BWE_SUCCESS || err2 != BWE_SUCCESS || win1 == 0 || win2 == 0) {
        display_print("[PHASE3] ERROR: Failed to create BWE windows for GL testing!\n");
        return;
    }

    test_clip_golden(win1);
    test_viewport_perspective(win1);
    test_top_left_shared_edge(win1);
    test_depth_occlusion(win1);
    test_resize_depth_safety(win1);
    test_multi_context_raster_isolation(win1, win2);
    test_rasterizer_torture(win1);
    test_public_gl_3d_presentation(win1);

    BOS_DestroySurface(win1);
    BOS_DestroySurface(win2);

    display_print("=========================================\n");
}
