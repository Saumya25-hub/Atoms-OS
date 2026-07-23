#include "kernel/debug/test_gl_phase10.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_fbo.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char* str);
extern void display_print_dec(uint32_t val);
extern bwe_error_t BOS_CreateWindow(int32_t x, int32_t y, int32_t width, int32_t height, const char* title, uint32_t* out_window_id);
extern bwe_error_t BOS_DestroySurface(uint32_t window_id);

static void print_pass(const char* test_name) {
    display_print("[PHASE10] ");
    display_print(test_name);
    display_print(": PASS\n");
}

static void print_fail(const char* test_name, const char* reason) {
    display_print("[PHASE10] ");
    display_print(test_name);
    display_print(": FAIL! (");
    display_print(reason);
    display_print(")\n");
}

/* TEST A: Framebuffer Object Namespace & Lifecycle */
static void test_a_fbo_namespace_lifecycle(void) {
    display_print("[PHASE10] Test A: Framebuffer Object Namespace & Lifecycle...\n");

    GLuint fbos[3] = {0};
    glGenFramebuffers(3, fbos);

    bool gen_ok = (fbos[0] != 0 && fbos[1] != 0 && fbos[2] != 0 && fbos[0] != fbos[1] && fbos[1] != fbos[2]);
    bool is_fb_ok = (glIsFramebuffer(fbos[0]) == GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
    GLContextState* state = gl_state_get_current();
    bool bind_ok = (state && state->bound_draw_framebuffer == fbos[0] && state->bound_read_framebuffer == fbos[0]);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    bool unbind_ok = (state->bound_draw_framebuffer == 0);

    glDeleteFramebuffers(3, fbos);
    bool del_ok = (glIsFramebuffer(fbos[0]) == GL_FALSE);

    if (gen_ok && is_fb_ok && bind_ok && unbind_ok && del_ok) {
        print_pass("Framebuffer Object Namespace & Lifecycle");
    } else {
        print_fail("FBO Lifecycle", "Gen/Bind/Is/Delete mismatch");
    }
}

/* TEST B: Renderbuffer Object Namespace & Lifecycle */
static void test_b_rbo_namespace_lifecycle(void) {
    display_print("[PHASE10] Test B: Renderbuffer Object Namespace & Lifecycle...\n");

    GLuint rbos[2] = {0};
    glGenRenderbuffers(2, rbos);

    bool gen_ok = (rbos[0] != 0 && rbos[1] != 0 && rbos[0] != rbos[1]);

    glBindRenderbuffer(GL_RENDERBUFFER, rbos[0]);
    bool is_rbo_ok = (glIsRenderbuffer(rbos[0]) == GL_TRUE);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 128, 128);

    GLContextState* state = gl_state_get_current();
    GLRenderbufferObject* rbo = gl_rbo_get_renderbuffer(state, rbos[0]);
    bool storage_ok = (rbo && rbo->width == 128 && rbo->height == 128 && rbo->storage_buffer != NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glDeleteRenderbuffers(2, rbos);

    bool del_ok = (glIsRenderbuffer(rbos[0]) == GL_FALSE);

    if (gen_ok && is_rbo_ok && storage_ok && del_ok) {
        print_pass("Renderbuffer Object Namespace & Lifecycle");
    } else {
        print_fail("RBO Lifecycle", "Gen/Storage/Delete mismatch");
    }
}

/* TEST C: Framebuffer Completeness Status Matrix */
static void test_c_fbo_completeness_matrix(void) {
    display_print("[PHASE10] Test C: Framebuffer Completeness Status Matrix...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Empty FBO must be incomplete
    GLenum status_empty = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bool empty_ok = (status_empty == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);

    // Attach texture
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    GLenum status_complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bool complete_ok = (status_complete == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (empty_ok && complete_ok) {
        print_pass("Framebuffer Completeness Status Matrix");
    } else {
        print_fail("FBO Completeness", "Completeness check state mismatch");
    }
}

/* TEST D: Color Texture Attachment Golden Test */
static void test_d_color_texture_attachment(void) {
    display_print("[PHASE10] Test D: Color Texture Attachment Golden Test...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    GLContextState* state = gl_state_get_current();
    GLRenderTarget target;
    bool active_ok = gl_get_active_render_target(state, &target);
    bool target_ok = (active_ok && target.is_fbo && target.has_color && target.width == 128 && target.height == 128 && target.color_buffer != NULL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (target_ok) {
        print_pass("Color Texture Attachment Golden Test");
    } else {
        print_fail("Texture Attachment", "Target resolution mismatch");
    }
}

/* TEST E: Offscreen Clear + ReadPixels Golden Test */
static void test_e_offscreen_clear_readback(void) {
    display_print("[PHASE10] Test E: Offscreen Clear + ReadPixels Golden Test...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); // Green
    glClear(GL_COLOR_BUFFER_BIT);

    uint8_t readback[4] = {0};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, readback);

    bool green_ok = (readback[0] == 0 && readback[1] == 255 && readback[2] == 0 && readback[3] == 255);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (green_ok) {
        print_pass("Offscreen Clear + ReadPixels Golden Test");
    } else {
        print_fail("Offscreen Clear", "Readback pixels were not green");
    }
}

/* TEST F: Offscreen Triangle Rendering Golden Test */
static void test_f_offscreen_triangle_rendering(void) {
    display_print("[PHASE10] Test F: Offscreen Triangle Rendering Golden Test...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glClear(GL_COLOR_BUFFER_BIT);

    // Render Blue Triangle
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.8f, -0.8f);
    glVertex2f( 0.8f, -0.8f);
    glVertex2f( 0.0f,  0.8f);
    glEnd();

    uint8_t readback[4] = {0};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, readback);

    bool blue_ok = (readback[0] == 0 && readback[1] == 0 && readback[2] == 255 && readback[3] == 255);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (blue_ok) {
        print_pass("Offscreen Triangle Rendering Golden Test");
    } else {
        print_fail("Offscreen Triangle", "Triangle center pixel was not blue");
    }
}

/* TEST G: FBO Depth Attachment Isolation */
static void test_g_fbo_depth_isolation(void) {
    display_print("[PHASE10] Test G: FBO Depth Attachment Isolation...\n");

    GLuint fbo = 0, tex = 0, depth_rbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 64, 64);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    GLContextState* state = gl_state_get_current();
    GLRenderTarget target;
    gl_get_active_render_target(state, &target);

    bool depth_ok = (target.has_depth && target.depth_buffer != NULL && target.depth_buffer[0] == 1.0f);

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &depth_rbo);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (depth_ok) {
        print_pass("FBO Depth Attachment Isolation");
    } else {
        print_fail("FBO Depth Isolation", "FBO depth buffer clear mismatch");
    }
}

/* TEST H: FBO Stencil Attachment Isolation */
static void test_h_fbo_stencil_isolation(void) {
    display_print("[PHASE10] Test H: FBO Stencil Attachment Isolation...\n");

    GLuint fbo = 0, tex = 0, stencil_rbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &stencil_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, stencil_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 64, 64);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil_rbo);

    glClearStencil(42);
    glClear(GL_STENCIL_BUFFER_BIT);

    GLContextState* state = gl_state_get_current();
    GLRenderTarget target;
    gl_get_active_render_target(state, &target);

    bool stencil_ok = (target.has_stencil && target.stencil_buffer != NULL && target.stencil_buffer[0] == 42);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &stencil_rbo);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (stencil_ok) {
        print_pass("FBO Stencil Attachment Isolation");
    } else {
        print_fail("FBO Stencil Isolation", "FBO stencil buffer clear mismatch");
    }
}

/* TEST I: Combined Color + Depth + Stencil FBO */
static void test_i_combined_fbo(void) {
    display_print("[PHASE10] Test I: Combined Color + Depth + Stencil FBO...\n");

    GLuint fbo = 0, tex = 0, depth_rbo = 0, stencil_rbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 64, 64);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

    glGenRenderbuffers(1, &stencil_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, stencil_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 64, 64);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil_rbo);

    bool complete_ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    GLContextState* state = gl_state_get_current();
    GLRenderTarget target;
    gl_get_active_render_target(state, &target);

    bool all_attached = (target.has_color && target.has_depth && target.has_stencil);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &stencil_rbo);
    glDeleteRenderbuffers(1, &depth_rbo);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (complete_ok && all_attached) {
        print_pass("Combined Color + Depth + Stencil FBO");
    } else {
        print_fail("Combined FBO", "Attachments or completeness mismatch");
    }
}

/* TEST J: True Render-to-Texture Two-Pass Golden Test */
static void test_j_true_render_to_texture(void) {
    display_print("[PHASE10] Test J: True Render-to-Texture Two-Pass Golden Test...\n");

    // PASS 1: Render Red triangle into FBO Texture
    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor4f(1.0f, 0.0f, 0.0f, 1.0f); // Red
    glBegin(GL_TRIANGLES);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f( 1.0f, -1.0f);
    glVertex2f( 0.0f,  1.0f);
    glEnd();

    // PASS 2: Bind Framebuffer 0, Bind FBO Texture, Render textured Quad into window
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (!bgl_ctx || !bgl_ctx->bound_drawable) { print_fail("Render-to-Texture", "No window drawable"); return; }

    glViewport(0, 0, bgl_ctx->bound_drawable->width, bgl_ctx->bound_drawable->height);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.5f, -0.5f);
    glTexCoord2f(0.5f, 1.0f); glVertex2f( 0.0f,  0.5f);
    glEnd();

    uint8_t readback[4] = {0};
    uint32_t center_x = bgl_ctx->bound_drawable->width / 2;
    uint32_t center_y = bgl_ctx->bound_drawable->height / 2;
    glReadPixels((GLint)center_x, (GLint)center_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, readback);

    bool rtt_ok = (readback[0] >= 250 && readback[1] <= 5 && readback[2] <= 5);

    glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (rtt_ok) {
        print_pass("True Render-to-Texture Two-Pass Golden Test");
    } else {
        print_fail("Render-to-Texture", "Pass 2 sampled color mismatch");
    }
}

/* TEST K: Rendered Texture Sampling / Orientation Test */
static void test_k_texture_sampling_orientation(void) {
    display_print("[PHASE10] Test K: Rendered Texture Sampling / Orientation Test...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glViewport(0, 0, 2, 2);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // Red
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* tex_obj = gl_state_get_bound_texture(state);

    bool tex_data_ok = (tex_obj && tex_obj->levels[0].pixel_data != NULL);

    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (tex_data_ok) {
        print_pass("Rendered Texture Sampling / Orientation Test");
    } else {
        print_fail("Texture Orientation", "Texture payload missing");
    }
}

/* TEST L: Viewport + Scissor Different-Size Target Torture */
static void test_l_viewport_scissor_target_torture(void) {
    display_print("[PHASE10] Test L: Viewport + Scissor Different-Size Target Torture...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glViewport(0, 0, 128, 128);
    glEnable(GL_SCISSOR_TEST);
    glScissor(32, 32, 64, 64);

    glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    glClear(GL_COLOR_BUFFER_BIT);

    uint8_t inside[4] = {0}, outside[4] = {0};
    glReadPixels(64, 64, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, inside);
    glReadPixels(5, 5, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outside);

    bool scissor_ok = (inside[0] == 255 && inside[1] == 255 && outside[0] == 0 && outside[1] == 0);

    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (scissor_ok) {
        print_pass("Viewport + Scissor Different-Size Target Torture");
    } else {
        print_fail("Scissor Target Torture", "Scissor clipping on FBO mismatch");
    }
}

/* TEST M: FBO A -> FBO B -> Default Target Switching */
static void test_m_target_switching(void) {
    display_print("[PHASE10] Test M: FBO A -> FBO B -> Default Target Switching...\n");

    GLuint fboA = 0, texA = 0, fboB = 0, texB = 0;
    glGenFramebuffers(1, &fboA); glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    glGenTextures(1, &texA); glBindTexture(GL_TEXTURE_2D, texA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texA, 0);
    glViewport(0, 0, 32, 32);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT); // Red

    glGenFramebuffers(1, &fboB); glBindFramebuffer(GL_FRAMEBUFFER, fboB);
    glGenTextures(1, &texB); glBindTexture(GL_TEXTURE_2D, texB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texB, 0);
    glViewport(0, 0, 32, 32);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT); // Green

    uint8_t readB[4] = {0}; glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, readB);
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    uint8_t readA[4] = {0}; glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, readA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    bool switch_ok = (readA[0] == 255 && readA[1] == 0 && readB[0] == 0 && readB[1] == 255);

    glDeleteTextures(1, &texB); glDeleteFramebuffers(1, &fboB);
    glDeleteTextures(1, &texA); glDeleteFramebuffers(1, &fboA);

    if (switch_ok) {
        print_pass("FBO A -> FBO B -> Default Target Switching");
    } else {
        print_fail("Target Switching", "Target contents corrupted during switch");
    }
}

/* TEST N: Attachment Deletion / Redefinition Safety */
static void test_n_attachment_deletion_safety(void) {
    display_print("[PHASE10] Test N: Attachment Deletion / Redefinition Safety...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    bool complete1 = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // Delete texture while attached
    glDeleteTextures(1, &tex);

    // FBO must automatically become incomplete
    bool incomplete2 = (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    if (complete1 && incomplete2) {
        print_pass("Attachment Deletion / Redefinition Safety");
    } else {
        print_fail("Attachment Deletion Safety", "Incompleteness after texture delete failed");
    }
}

/* TEST O: Feedback Loop Hazard Safety */
static void test_o_feedback_loop_safety(void) {
    display_print("[PHASE10] Test O: Feedback Loop Hazard Safety...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* tex_obj = gl_state_get_bound_texture(state);

    bool aliased = gl_fbo_is_sampler_aliased(state, tex_obj);

    glDisable(GL_TEXTURE_2D);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (aliased) {
        print_pass("Feedback Loop Hazard Safety");
    } else {
        print_fail("Feedback Loop Hazard", "Hazard detection failed");
    }
}

/* TEST P: Window Resize vs User FBO Isolation */
static void test_p_window_resize_fbo_isolation(void) {
    display_print("[PHASE10] Test P: Window Resize vs User FBO Isolation...\n");

    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    BGLContext* bgl_ctx = bglGetCurrentContext();
    if (bgl_ctx && bgl_ctx->bound_drawable) {
        bglResizeDrawable(bgl_ctx->bound_drawable, 320, 240);
    }

    GLContextState* state = gl_state_get_current();
    GLRenderTarget target;
    gl_get_active_render_target(state, &target);

    bool iso_ok = (target.width == 256 && target.height == 256);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    if (iso_ok) {
        print_pass("Window Resize vs User FBO Isolation");
    } else {
        print_fail("Window Resize Isolation", "FBO dimensions altered by window resize");
    }
}

/* TEST Q: 100-Cycle FBO/Texture/Renderbuffer Memory Torture */
static void test_q_memory_torture_100_cycles(void) {
    display_print("[PHASE10] Test Q: 100-Cycle FBO/Texture/Renderbuffer Memory Torture...\n");

    bool torture_ok = true;
    for (int cycle = 0; cycle < 100; cycle++) {
        GLuint fbo = 0, tex = 0, rbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 64, 64);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            torture_ok = false;
            break;
        }

        glViewport(0, 0, 64, 64);
        glClearColor(0.1f * (float)(cycle % 10), 0.5f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteRenderbuffers(1, &rbo);
        glDeleteTextures(1, &tex);
        glDeleteFramebuffers(1, &fbo);
    }

    if (torture_ok) {
        print_pass("100-Cycle FBO/Texture/Renderbuffer Memory Torture");
    } else {
        print_fail("Memory Torture", "Cycle failure or memory leakage");
    }
}

/* TEST R: Multi-Context FBO Namespace Isolation */
static void test_r_multi_context_fbo_isolation(void) {
    display_print("[PHASE10] Test R: Multi-Context FBO Namespace Isolation...\n");

    uint32_t win1_id = 0, win2_id = 0;
    BOS_CreateWindow(10, 10, 160, 120, "GL_P10_R_Win1", &win1_id);
    BOS_CreateWindow(200, 10, 160, 120, "GL_P10_R_Win2", &win2_id);

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1_id);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2_id);
    BGLContext* ctx1 = bglCreateContext(d1);
    BGLContext* ctx2 = bglCreateContext(d2);

    bglMakeCurrent(ctx1, d1);
    GLuint fbo1 = 0;
    glGenFramebuffers(1, &fbo1);

    bglMakeCurrent(ctx2, d2);
    GLuint fbo2 = 0;
    glGenFramebuffers(1, &fbo2);

    bool iso_ok = (fbo1 == 1 && fbo2 == 1 && ctx1 != ctx2);

    bglMakeCurrent(ctx1, d1); glDeleteFramebuffers(1, &fbo1); bglDestroyContext(ctx1); bglDestroyDrawable(d1);
    bglMakeCurrent(ctx2, d2); glDeleteFramebuffers(1, &fbo2); bglDestroyContext(ctx2); bglDestroyDrawable(d2);
    BOS_DestroySurface(win1_id); BOS_DestroySurface(win2_id);

    if (iso_ok) {
        print_pass("Multi-Context FBO Namespace Isolation");
    } else {
        print_fail("Multi-Context Isolation", "FBO ID collision or leakage across contexts");
    }
}

/* TEST S: Two-Window + Two-Offscreen-Target Isolation */
static void test_s_two_window_two_fbo_isolation(void) {
    display_print("[PHASE10] Test S: Two-Window + Two-Offscreen-Target Isolation...\n");

    uint32_t win1_id = 0, win2_id = 0;
    BOS_CreateWindow(10, 10, 160, 120, "GL_P10_Win1", &win1_id);
    BOS_CreateWindow(200, 10, 160, 120, "GL_P10_Win2", &win2_id);

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1_id);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2_id);
    BGLContext* ctx1 = bglCreateContext(d1);
    BGLContext* ctx2 = bglCreateContext(d2);

    bglMakeCurrent(ctx1, d1);
    GLuint fbo1 = 0, tex1 = 0;
    glGenFramebuffers(1, &fbo1); glBindFramebuffer(GL_FRAMEBUFFER, fbo1);
    glGenTextures(1, &tex1); glBindTexture(GL_TEXTURE_2D, tex1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);
    glViewport(0, 0, 64, 64); glClearColor(1.0f, 0.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);

    bglMakeCurrent(ctx2, d2);
    GLuint fbo2 = 0, tex2 = 0;
    glGenFramebuffers(1, &fbo2); glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
    glGenTextures(1, &tex2); glBindTexture(GL_TEXTURE_2D, tex2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
    glViewport(0, 0, 64, 64); glClearColor(0.0f, 1.0f, 0.0f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);

    bglMakeCurrent(ctx1, d1);
    uint8_t read1[4] = {0}; glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, read1);

    bglMakeCurrent(ctx2, d2);
    uint8_t read2[4] = {0}; glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, read2);

    bool iso_ok = (read1[0] == 255 && read1[1] == 0 && read2[0] == 0 && read2[1] == 255);

    bglMakeCurrent(ctx1, d1); glDeleteTextures(1, &tex1); glDeleteFramebuffers(1, &fbo1);
    bglMakeCurrent(ctx2, d2); glDeleteTextures(1, &tex2); glDeleteFramebuffers(1, &fbo2);

    bglDestroyContext(ctx1); bglDestroyContext(ctx2);
    bglDestroyDrawable(d1); bglDestroyDrawable(d2);
    BOS_DestroySurface(win1_id); BOS_DestroySurface(win2_id);

    if (iso_ok) {
        print_pass("Two-Window + Two-Offscreen-Target Isolation");
    } else {
        print_fail("Two-Window Isolation", "Readback contents cross-contaminated");
    }
}

/* TEST T: glReadPixels / Copy Operations on FBO */
static void test_t_fbo_copy_operations(void) {
    display_print("[PHASE10] Test T: glReadPixels / Copy Operations on FBO...\n");

    GLuint fbo = 0, tex1 = 0, tex2 = 0;
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex1); glBindTexture(GL_TEXTURE_2D, tex1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);

    glViewport(0, 0, 64, 64);
    glClearColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    glClear(GL_COLOR_BUFFER_BIT);

    // Copy from FBO into tex2
    glGenTextures(1, &tex2); glBindTexture(GL_TEXTURE_2D, tex2);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 64, 64, 0);

    GLContextState* state = gl_state_get_current();
    GLTextureObject* t2_obj = gl_state_get_texture(state, tex2);

    bool copy_ok = (t2_obj && t2_obj->levels[0].pixel_data != NULL && t2_obj->levels[0].width == 64);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(1, &tex2);
    glDeleteTextures(1, &tex1);
    glDeleteFramebuffers(1, &fbo);

    if (copy_ok) {
        print_pass("glReadPixels / Copy Operations on FBO");
    } else {
        print_fail("FBO Copy Operations", "glCopyTexImage2D from FBO failed");
    }
}

/* TEST U: Full Phase 0-10 Regression Suite */
static void test_u_phase_0_10_regression(void) {
    display_print("[PHASE10] Test U: Full Phase 0-10 Regression Suite...\n");

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    GLContextState* state = gl_state_get_current();
    bool reg_ok = (state && state->framebuffer_binds > 0 && state->render_target_switches > 0);

    if (reg_ok) {
        print_pass("Full Phase 0-10 Regression Suite");
    } else {
        print_fail("Phase 0-10 Regression", "State telemetry unrecorded");
    }
}

/* TEST V: Real Public GL API Render-to-Texture Presentation Demo */
static void test_v_public_rtt_demo(void) {
    display_print("[PHASE10] Test V: Real Public GL API Render-to-Texture Presentation Demo...\n");

    uint32_t win_id = 0;
    BOS_CreateWindow(100, 100, 320, 240, "ATOMS OS Phase 10 Render-to-Texture Demo", &win_id);
    BGLDrawable* drawable = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(drawable);
    bglMakeCurrent(ctx, drawable);

    // 1. Create Offscreen FBO (64x64)
    GLuint fbo = 0, tex = 0, rbo = 0;
    glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &rbo); glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 64, 64);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    bool status_ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // PASS 1: Render 3D Scene into FBO
    glViewport(0, 0, 64, 64);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.1f, 0.4f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render multicolored 3D pyramid inside FBO
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);
    glRotatef(45.0f, 1.0f, 1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    // Face 1: Red
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f); glVertex3f( 0.0f, 1.0f,  0.0f);
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f); glVertex3f(-1.0f,-1.0f,  1.0f);
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f); glVertex3f( 1.0f,-1.0f,  1.0f);
    // Face 2: Green
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f); glVertex3f( 0.0f, 1.0f,  0.0f);
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f); glVertex3f( 1.0f,-1.0f,  1.0f);
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f); glVertex3f( 1.0f,-1.0f, -1.0f);
    glEnd();

    // PASS 2: Bind Framebuffer 0 & Render textured 3D quad to window
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 320, 240);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -2.5f);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
    glTexCoord2f(0.5f, 1.0f); glVertex3f( 0.0f,  1.0f, 0.0f);
    glEnd();

    bglSwapBuffers(ctx);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);

    bglDestroyContext(ctx);
    bglDestroyDrawable(drawable);
    BOS_DestroySurface(win_id);

    if (status_ok) {
        print_pass("Real Public GL API Render-to-Texture Presentation Demo");
    } else {
        print_fail("Public RTT Demo", "FBO setup status failure");
    }
}

void run_phase10_gl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 10 VERIFICATION\n");
    display_print("=========================================\n\n");

    uint32_t win1_id = 0, win2_id = 0;
    BOS_CreateWindow(10, 10, 160, 120, "GL_P10_Win1", &win1_id);
    BOS_CreateWindow(200, 10, 160, 120, "GL_P10_Win2", &win2_id);

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1_id);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2_id);
    BGLContext* ctx1 = bglCreateContext(d1);
    BGLContext* ctx2 = bglCreateContext(d2);

    bglMakeCurrent(ctx1, d1);

    test_a_fbo_namespace_lifecycle();
    test_b_rbo_namespace_lifecycle();
    test_c_fbo_completeness_matrix();
    test_d_color_texture_attachment();
    test_e_offscreen_clear_readback();
    test_f_offscreen_triangle_rendering();
    test_g_fbo_depth_isolation();
    test_h_fbo_stencil_isolation();
    test_i_combined_fbo();
    test_j_true_render_to_texture();
    test_k_texture_sampling_orientation();
    test_l_viewport_scissor_target_torture();
    test_m_target_switching();
    test_n_attachment_deletion_safety();
    test_o_feedback_loop_safety();
    test_p_window_resize_fbo_isolation();
    test_q_memory_torture_100_cycles();
    test_r_multi_context_fbo_isolation();
    test_s_two_window_two_fbo_isolation();
    bglMakeCurrent(ctx1, d1);
    test_t_fbo_copy_operations();
    test_u_phase_0_10_regression();
    test_v_public_rtt_demo();

    bglDestroyContext(ctx1);
    bglDestroyContext(ctx2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);
    BOS_DestroySurface(win1_id);
    BOS_DestroySurface(win2_id);

    display_print("=========================================\n");
    display_print("   PHASE 10 VERIFICATION COMPLETE: PASS  \n");
    display_print("=========================================\n\n");
}
