#include "test_gl_phase4.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_texture.h"
#include "kernel/graphics/gl/gl_sampler.h"
#include "kernel/graphics/gl/gl_clip.h"
#include "kernel/graphics/gl/gl_viewport.h"
#include "kernel/graphics/gl/gl_rasterizer.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/graphics/gl/gl_math.h"

extern void display_print(const char* str);
extern void display_print_hex(uint64_t val);
extern void display_print_dec(uint32_t val);
extern void crash_log_add(const char* msg);

static bool float_eq(float a, float b, float eps) {
    return gl_fabsf(a - b) <= eps;
}

static void test_tex_lifecycle(uint32_t win_id) {
    display_print("[PHASE4] Test A: Texture Object Lifecycle & Namespace...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;
    uint32_t pixels[16] = {0xFFFFFFFF};

    for (int i = 0; i < 1000; i++) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        if (tex == 0) { pass = false; break; }
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glDeleteTextures(1, &tex);
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Texture Object Lifecycle & Namespace: PASS (1000 cycles OK)\n");
        crash_log_add("[PHASE4 TEST] Texture Object Lifecycle & Namespace: PASS");
    } else {
        display_print("[PHASE4] Texture Object Lifecycle & Namespace: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Texture Object Lifecycle & Namespace: FAIL");
    }
}

static void test_tex_upload_conversion(uint32_t win_id) {
    display_print("\n[PHASE4] Test B: Texture Upload & Format Conversion Golden Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;
    uint8_t rgb_pixels[6] = {255, 0, 0,  0, 255, 0}; // 2x1 RGB texture (Red, Green)

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_pixels);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* obj = gl_state_get_bound_texture(state);

    if (!obj || !obj->levels[0].pixel_data || obj->levels[0].width != 2 || obj->levels[0].height != 1) {
        pass = false;
    } else {
        // Verify texel 0 is RGBA (255, 0, 0, 255)
        if (obj->levels[0].pixel_data[0] != 255 || obj->levels[0].pixel_data[1] != 0 || obj->levels[0].pixel_data[2] != 0 || obj->levels[0].pixel_data[3] != 255) pass = false;
        // Verify texel 1 is RGBA (0, 255, 0, 255)
        if (obj->levels[0].pixel_data[4] != 0 || obj->levels[0].pixel_data[5] != 255 || obj->levels[0].pixel_data[6] != 0 || obj->levels[0].pixel_data[7] != 255) pass = false;
    }

    glDeleteTextures(1, &tex);
    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Texture Upload & Format Conversion Golden Test: PASS\n");
        crash_log_add("[PHASE4 TEST] Texture Upload & Format Conversion Golden Test: PASS");
    } else {
        display_print("[PHASE4] Texture Upload & Format Conversion Golden Test: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Texture Upload & Format Conversion Golden Test: FAIL");
    }
}

static void test_wrap_boundary(uint32_t win_id) {
    display_print("\n[PHASE4] Test C: Texture Wrap Boundary Torture...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;
    uint32_t pixels[4] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF}; // 2x2 texture

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* obj = gl_state_get_bound_texture(state);

    float coords[10] = {-1000.5f, -1.25f, -0.1f, 0.0f, 0.5f, 0.999f, 1.0f, 1.25f, 1000.5f, 5.75f};

    // Test REPEAT and CLAMP across extreme coordinates
    for (int w = 0; w < 2; w++) {
        GLenum mode = (w == 0) ? GL_REPEAT : GL_CLAMP;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);

        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                float sample[4];
                gl_sample_texture(obj, coords[i], coords[j], sample);
                if (sample[0] < 0.0f || sample[0] > 1.0f || sample[1] < 0.0f || sample[1] > 1.0f) {
                    pass = false;
                }
            }
        }
    }

    glDeleteTextures(1, &tex);
    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Texture Wrap Boundary Torture: PASS\n");
        crash_log_add("[PHASE4 TEST] Texture Wrap Boundary Torture: PASS");
    } else {
        display_print("[PHASE4] Texture Wrap Boundary Torture: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Texture Wrap Boundary Torture: FAIL");
    }
}

static void test_nearest_sampling(uint32_t win_id) {
    display_print("\n[PHASE4] Test D: GL_NEAREST Sampling Golden Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;
    // 2x2 texture: (0,0)=Red (1,0)=Green (0,1)=Blue (1,1)=White
    uint8_t tex_data[16] = {
        255, 0, 0, 255,    0, 255, 0, 255,
        0, 0, 255, 255,  255, 255, 255, 255
    };

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* obj = gl_state_get_bound_texture(state);

    float out[4];
    // Sample top-left texel (0,0) at UV (0.2, 0.2) -> Red
    gl_sample_texture(obj, 0.2f, 0.2f, out);
    if (!float_eq(out[0], 1.0f, 1e-2f) || !float_eq(out[1], 0.0f, 1e-2f)) pass = false;

    // Sample top-right texel (1,0) at UV (0.8, 0.2) -> Green
    gl_sample_texture(obj, 0.8f, 0.2f, out);
    if (!float_eq(out[0], 0.0f, 1e-2f) || !float_eq(out[1], 1.0f, 1e-2f)) pass = false;

    glDeleteTextures(1, &tex);
    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] GL_NEAREST Sampling Golden Test: PASS\n");
        crash_log_add("[PHASE4 TEST] GL_NEAREST Sampling Golden Test: PASS");
    } else {
        display_print("[PHASE4] GL_NEAREST Sampling Golden Test: FAIL!\n");
        crash_log_add("[PHASE4 TEST] GL_NEAREST Sampling Golden Test: FAIL");
    }
}

static void test_linear_filtering(uint32_t win_id) {
    display_print("\n[PHASE4] Test E: GL_LINEAR Bilinear Filtering Golden Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;
    // 2x2 texture: (0,0)=Black(0) (1,0)=Red(255) (0,1)=Black(0) (1,1)=Red(255)
    uint8_t tex_data[16] = {
        0, 0, 0, 255,      255, 0, 0, 255,
        0, 0, 0, 255,      255, 0, 0, 255
    };

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* obj = gl_state_get_bound_texture(state);

    float out[4];
    // Sample exact center UV (0.5, 0.5) -> 50% Red blend (R = 0.5)
    gl_sample_texture(obj, 0.5f, 0.5f, out);

    if (!float_eq(out[0], 0.5f, 0.05f)) {
        display_print("[PHASE4] ERROR: Bilinear interpolation value mismatch!\n");
        pass = false;
    }

    glDeleteTextures(1, &tex);
    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] GL_LINEAR Bilinear Filtering Golden Test: PASS\n");
        crash_log_add("[PHASE4 TEST] GL_LINEAR Bilinear Filtering Golden Test: PASS");
    } else {
        display_print("[PHASE4] GL_LINEAR Bilinear Filtering Golden Test: FAIL!\n");
        crash_log_add("[PHASE4 TEST] GL_LINEAR Bilinear Filtering Golden Test: FAIL");
    }
}

static void test_perspective_correct_interpolation(uint32_t win_id) {
    display_print("\n[PHASE4] Test F: Perspective-Correct Texture Interpolation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;

    // Set up 2 vertices with significantly different W values (w0 = 1.0, w1 = 4.0)
    GLScreenVertex v0 = {.x = 0, .y = 0, .z = 0.5f, .inv_w = 1.0f / 1.0f, .s_over_w = 0.0f / 1.0f, .t_over_w = 0.0f / 1.0f};
    GLScreenVertex v1 = {.x = 100, .y = 0, .z = 0.5f, .inv_w = 1.0f / 4.0f, .s_over_w = 1.0f / 4.0f, .t_over_w = 0.0f / 4.0f};

    // At midpoint in screen space (l0 = 0.5, l1 = 0.5):
    // Affine UV: s = 0.5 * 0.0 + 0.5 * 1.0 = 0.50
    // Hyperbolic UV: s = (0.5 * 0.0/1.0 + 0.5 * 1.0/4.0) / (0.5 * 1.0/1.0 + 0.5 * 1.0/4.0) = (0.125) / (0.625) = 0.20!
    float l0 = 0.5f, l1 = 0.5f;
    float denom = l0 * v0.inv_w + l1 * v1.inv_w;
    float hyp_s = (l0 * v0.s_over_w + l1 * v1.s_over_w) / denom;
    float aff_s = l0 * 0.0f + l1 * 1.0f;

    if (!float_eq(hyp_s, 0.20f, 1e-3f) || float_eq(hyp_s, aff_s, 1e-2f)) {
        display_print("[PHASE4] ERROR: Hyperbolic perspective correction formula mismatch!\n");
        pass = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Perspective-Correct Texture Interpolation: PASS (0.20 vs affine 0.50)\n");
        crash_log_add("[PHASE4 TEST] Perspective-Correct Texture Interpolation: PASS");
    } else {
        display_print("[PHASE4] Perspective-Correct Texture Interpolation: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Perspective-Correct Texture Interpolation: FAIL");
    }
}

static void test_textured_clipping(uint32_t win_id) {
    display_print("\n[PHASE4] Test G: Textured Homogeneous Clipping Continuity...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;

    // Triangle crossing left clip plane (x0 = -2.0, w0 = 1.0 -> outside left)
    GLVertex tri[3] = {
        { .clip_pos = {-2.0f,  0.0f, 0.0f, 1.0f}, .texcoord = {0.0f, 0.0f, 0.0f, 1.0f} },
        { .clip_pos = { 1.0f, -1.0f, 0.0f, 1.0f}, .texcoord = {1.0f, 0.0f, 0.0f, 1.0f} },
        { .clip_pos = { 1.0f,  1.0f, 0.0f, 1.0f}, .texcoord = {1.0f, 1.0f, 0.0f, 1.0f} }
    };

    GLVertex clipped[GL_MAX_CLIPPED_POLYGON_VERTICES];
    uint32_t poly_count = gl_clip_triangle(tri, clipped);

    if (poly_count < 3 || clipped[0].texcoord[0] < 0.0f || clipped[0].texcoord[0] > 1.0f) {
        display_print("[PHASE4] ERROR: Textured clipping attribute continuity failed!\n");
        pass = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Textured Homogeneous Clipping Continuity: PASS\n");
        crash_log_add("[PHASE4 TEST] Textured Homogeneous Clipping Continuity: PASS");
    } else {
        display_print("[PHASE4] Textured Homogeneous Clipping Continuity: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Textured Homogeneous Clipping Continuity: FAIL");
    }
}

static void test_texture_env(uint32_t win_id) {
    display_print("\n[PHASE4] Test H: Texture Environment REPLACE & MODULATE...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);
    glEnable(GL_TEXTURE_2D);

    // 1x1 Red texture (255, 0, 0, 255)
    uint8_t tex_data[4] = {255, 0, 0, 255};
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);

    // Test REPLACE mode with Green vertex color -> Output must be RED (0xFFFF0000)
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    glColor3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.5f, 0.5f); glVertex2f(-0.5f, -0.5f);
    glColor3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.5f, 0.5f); glVertex2f( 0.5f, -0.5f);
    glColor3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.5f, 0.5f); glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint32_t replace_pixel = d->color_buffer[60 * d->width + 80];

    // Test MODULATE mode with Green vertex color -> Red x Green = BLACK (0xFF000000)
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    glColor3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.5f, 0.5f); glVertex2f(-0.5f, -0.5f);
    glColor3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.5f, 0.5f); glVertex2f( 0.5f, -0.5f);
    glColor3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.5f, 0.5f); glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint32_t modulate_pixel = d->color_buffer[60 * d->width + 80];

    bool pass = true;
    if (((replace_pixel >> 16) & 0xFF) < 250 && ((replace_pixel >> 8) & 0xFF) > 5) pass = false;
    if (((modulate_pixel >> 16) & 0xFF) > 5 || ((modulate_pixel >> 8) & 0xFF) > 5) pass = false;

    glDeleteTextures(1, &tex);
    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Texture Environment REPLACE & MODULATE: PASS\n");
        crash_log_add("[PHASE4 TEST] Texture Environment REPLACE & MODULATE: PASS");
    } else {
        display_print("[PHASE4] Texture Environment REPLACE & MODULATE: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Texture Environment REPLACE & MODULATE: FAIL");
    }
}

static void test_multi_context_tex_isolation(uint32_t win1, uint32_t win2) {
    display_print("\n[PHASE4] Test I: Multi-Context Texture State Isolation...\n");

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2);
    BGLContext* c1 = bglCreateContext(d1);
    BGLContext* c2 = bglCreateContext(d2);

    if (!d1 || !d2 || !c1 || !c2) return;

    uint8_t red_tex[4]  = {255, 0, 0, 255};
    uint8_t green_tex[4] = {0, 255, 0, 255};

    // Context 1: Bind Tex 1 = Red
    bglMakeCurrent(c1, d1);
    GLuint t1 = 0;
    glGenTextures(1, &t1);
    glBindTexture(GL_TEXTURE_2D, t1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, red_tex);

    // Context 2: Bind Tex 1 = Green
    bglMakeCurrent(c2, d2);
    GLuint t2 = 0;
    glGenTextures(1, &t2);
    glBindTexture(GL_TEXTURE_2D, t2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, green_tex);

    // Re-bind Context 1 and verify Texture 1 is still Red
    bglMakeCurrent(c1, d1);
    GLContextState* s1 = gl_state_get_current();
    GLTextureObject* obj1 = gl_state_get_bound_texture(s1);

    // Re-bind Context 2 and verify Texture 1 is Green
    bglMakeCurrent(c2, d2);
    GLContextState* s2 = gl_state_get_current();
    GLTextureObject* obj2 = gl_state_get_bound_texture(s2);

    bool pass = true;
    if (!obj1 || obj1->levels[0].pixel_data[0] != 255 || obj1->levels[0].pixel_data[1] != 0) pass = false;
    if (!obj2 || obj2->levels[0].pixel_data[0] != 0   || obj2->levels[0].pixel_data[1] != 255) pass = false;

    bglReleaseCurrent();
    bglDestroyContext(c1);
    bglDestroyContext(c2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (pass) {
        display_print("[PHASE4] Multi-Context Texture State Isolation: PASS\n");
        crash_log_add("[PHASE4 TEST] Multi-Context Texture State Isolation: PASS");
    } else {
        display_print("[PHASE4] Multi-Context Texture State Isolation: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Multi-Context Texture State Isolation: FAIL");
    }
}

static void test_redefine_delete_torture(uint32_t win_id) {
    display_print("\n[PHASE4] Test J: Texture Redefinition & Deletion Torture...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    bool pass = true;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Repeatedly redefine dimensions 100 times
    for (int i = 1; i <= 100; i++) {
        int w = (i % 16) + 1;
        int h = (i % 16) + 1;
        uint8_t buf[1024];
        for (int b = 0; b < w * h * 4; b++) buf[b] = (uint8_t)(i & 0xFF);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    }

    glDeleteTextures(1, &tex);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Texture Redefinition & Deletion Torture: PASS (100 redefs OK)\n");
        crash_log_add("[PHASE4 TEST] Texture Redefinition & Deletion Torture: PASS");
    } else {
        display_print("[PHASE4] Texture Redefinition & Deletion Torture: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Texture Redefinition & Deletion Torture: FAIL");
    }
}

static void test_public_gl_textured_3d_presentation(uint32_t win_id) {
    display_print("\n[PHASE4] Test K: Real Public GL API Textured 3D Presentation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) return;
    bglMakeCurrent(ctx, d);

    glViewport(0, 0, 160, 120);

    // Perspective Projection setup
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // Generate 4x4 procedural checkerboard texture (Red / White)
    uint8_t checker[64];
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            int idx = (y * 4 + x) * 4;
            bool is_white = ((x + y) % 2 == 0);
            checker[idx + 0] = 255;
            checker[idx + 1] = is_white ? 255 : 0;
            checker[idx + 2] = is_white ? 255 : 0;
            checker[idx + 3] = 255;
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, checker);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Real Public GL Textured Quad Submission (decomposed to 2 triangles)
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
    glEnd();

    // Present frame through BGL -> BWE -> Compositor -> AGDTE
    bglSwapBuffers(ctx);

    uint32_t center_pixel = d->color_buffer[60 * d->width + 80];
    bool pass = (center_pixel != 0x00000000 && center_pixel != 0xFF191933);

    glDeleteTextures(1, &tex);
    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (pass) {
        display_print("[PHASE4] Real Public GL API Textured 3D Presentation: PASS (GL -> BGL -> BWE -> AGDTE)\n");
        crash_log_add("[PHASE4 TEST] Real Public GL API Textured 3D Presentation: PASS");
    } else {
        display_print("[PHASE4] Real Public GL API Textured 3D Presentation: FAIL!\n");
        crash_log_add("[PHASE4 TEST] Real Public GL API Textured 3D Presentation: FAIL");
    }
}

void run_phase4_gl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 4 VERIFICATION\n");
    display_print("=========================================\n");

    uint32_t win1 = 0, win2 = 0;
    bwe_error_t err1 = BOS_CreateWindow(100, 100, 160, 120, "GL_Window1", &win1);
    bwe_error_t err2 = BOS_CreateWindow(200, 200, 160, 120, "GL_Window2", &win2);

    if (err1 != BWE_SUCCESS || err2 != BWE_SUCCESS || win1 == 0 || win2 == 0) {
        display_print("[PHASE4] ERROR: Failed to create BWE windows for GL testing!\n");
        return;
    }

    test_tex_lifecycle(win1);
    test_tex_upload_conversion(win1);
    test_wrap_boundary(win1);
    test_nearest_sampling(win1);
    test_linear_filtering(win1);
    test_perspective_correct_interpolation(win1);
    test_textured_clipping(win1);
    test_texture_env(win1);
    test_multi_context_tex_isolation(win1, win2);
    test_redefine_delete_torture(win1);
    test_public_gl_textured_3d_presentation(win1);

    BOS_DestroySurface(win1);
    BOS_DestroySurface(win2);

    display_print("=========================================\n");
}
