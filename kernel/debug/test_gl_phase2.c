#include "kernel/debug/test_gl_phase2.h"
#include "kernel/graphics/bgl/bgl.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_math.h"
#include "kernel/graphics/gl/gl_pipeline.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/crash_log.h"
#include "kernel/wm/bwe/include/bwe.h"

static bool float_eq(float a, float b, float eps) {
    float diff = a - b;
    if (diff < 0.0f) diff = -diff;
    return diff <= eps;
}

static void test_gl_error_machine(uint32_t win_id) {
    display_print("[PHASE2] Test A: GL Error State Machine & Legality Check...\n");
    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) {
        display_print("[PHASE2] ERROR: Failed to create objects for error machine test!\n");
        return;
    }
    bglMakeCurrent(ctx, d);

    bool passed = true;

    // 1. Initial state must be GL_NO_ERROR
    if (glGetError() != GL_NO_ERROR) {
        display_print("[PHASE2] ERROR: Initial error state is not GL_NO_ERROR!\n");
        passed = false;
    }

    // 2. Invalid Enum Check
    glMatrixMode(0x9999);
    if (glGetError() != GL_INVALID_ENUM) {
        display_print("[PHASE2] ERROR: Invalid enum error not set!\n");
        passed = false;
    }

    // 3. Sticky error semantics check (subsequent call should return GL_NO_ERROR)
    if (glGetError() != GL_NO_ERROR) {
        display_print("[PHASE2] ERROR: Error state did not reset after glGetError!\n");
        passed = false;
    }

    // 4. glEnd without glBegin
    glEnd();
    if (glGetError() != GL_INVALID_OPERATION) {
        display_print("[PHASE2] ERROR: Unmatched glEnd did not set GL_INVALID_OPERATION!\n");
        passed = false;
    }

    // 5. Nested glBegin
    glBegin(GL_TRIANGLES);
    glBegin(GL_LINES);
    if (glGetError() != GL_INVALID_OPERATION) {
        display_print("[PHASE2] ERROR: Nested glBegin did not set GL_INVALID_OPERATION!\n");
        passed = false;
    }

    // 6. Illegal operation inside glBegin
    glMatrixMode(GL_PROJECTION);
    if (glGetError() != GL_INVALID_OPERATION) {
        display_print("[PHASE2] ERROR: glMatrixMode inside glBegin did not set GL_INVALID_OPERATION!\n");
        passed = false;
    }
    glEnd();
    glGetError(); // Clear error state

    // 7. Stack underflow
    glPopMatrix();
    if (glGetError() != GL_STACK_UNDERFLOW) {
        display_print("[PHASE2] ERROR: glPopMatrix at base stack did not set GL_STACK_UNDERFLOW!\n");
        passed = false;
    }

    // 8. Invalid viewport
    glViewport(0, 0, -10, 100);
    if (glGetError() != GL_INVALID_VALUE) {
        display_print("[PHASE2] ERROR: Negative viewport width did not set GL_INVALID_VALUE!\n");
        passed = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (passed) {
        display_print("[PHASE2] Error Machine & Legality Test: PASS\n");
        crash_log_add("[PHASE2 TEST] Error Machine & Legality Test: PASS");
    } else {
        display_print("[PHASE2] Error Machine & Legality Test: FAIL!\n");
        crash_log_add("[PHASE2 TEST] Error Machine & Legality Test: FAIL");
    }
}

static void test_gl_matrix_transformations(uint32_t win_id) {
    display_print("\n[PHASE2] Test B: Matrix Stack & Transformation Golden Tests...\n");
    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) {
        display_print("[PHASE2] ERROR: Failed to create objects for matrix test!\n");
        return;
    }
    bglMakeCurrent(ctx, d);

    bool passed = true;
    const float eps = 1e-3f;

    // 1. Identity Transformation
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLContextState* state = gl_state_get_current();
    GLVertex v;
    v.obj_pos = (GLVec4){1.0f, 2.0f, 3.0f, 1.0f};
    gl_pipeline_transform_vertex(&v, &state->modelview_stack[state->modelview_top], &state->projection_stack[state->projection_top]);

    if (!float_eq(v.clip_pos.x, 1.0f, eps) || !float_eq(v.clip_pos.y, 2.0f, eps) ||
        !float_eq(v.clip_pos.z, 3.0f, eps) || !float_eq(v.clip_pos.w, 1.0f, eps)) {
        display_print("[PHASE2] ERROR: Identity transformation failed!\n");
        passed = false;
    }

    // 2. Translation Test
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(10.0f, -5.0f, 2.0f);
    gl_pipeline_transform_vertex(&v, &state->modelview_stack[state->modelview_top], &state->projection_stack[state->projection_top]);

    if (!float_eq(v.eye_pos.x, 11.0f, eps) || !float_eq(v.eye_pos.y, -3.0f, eps) || !float_eq(v.eye_pos.z, 5.0f, eps)) {
        display_print("[PHASE2] ERROR: Translation transformation failed!\n");
        passed = false;
    }

    // 3. Scaling Test
    glLoadIdentity();
    glScalef(2.0f, 3.0f, 0.5f);
    gl_pipeline_transform_vertex(&v, &state->modelview_stack[state->modelview_top], &state->projection_stack[state->projection_top]);

    if (!float_eq(v.eye_pos.x, 2.0f, eps) || !float_eq(v.eye_pos.y, 6.0f, eps) || !float_eq(v.eye_pos.z, 1.5f, eps)) {
        display_print("[PHASE2] ERROR: Scaling transformation failed!\n");
        passed = false;
    }

    // 4. Rotation Test (90 degrees around Z axis)
    glLoadIdentity();
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    GLVertex v_rot;
    v_rot.obj_pos = (GLVec4){1.0f, 0.0f, 0.0f, 1.0f};
    gl_pipeline_transform_vertex(&v_rot, &state->modelview_stack[state->modelview_top], &state->projection_stack[state->projection_top]);

    // (1,0,0) rotated 90 deg around +Z -> (0,1,0)
    if (!float_eq(v_rot.eye_pos.x, 0.0f, eps) || !float_eq(v_rot.eye_pos.y, 1.0f, eps) || !float_eq(v_rot.eye_pos.z, 0.0f, eps)) {
        display_print("[PHASE2] ERROR: 90-degree Z-rotation failed!\n");
        passed = false;
    }

    // 5. Ortho Projection Test
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-10.0, 10.0, -10.0, 10.0, 1.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLVertex v_ortho;
    v_ortho.obj_pos = (GLVec4){5.0f, -5.0f, -10.0f, 1.0f};
    gl_pipeline_transform_vertex(&v_ortho, &state->modelview_stack[state->modelview_top], &state->projection_stack[state->projection_top]);

    // Ortho X NDC: 5 / 10 = 0.5
    // Ortho Y NDC: -5 / 10 = -0.5
    if (!float_eq(v_ortho.clip_pos.x, 0.5f, eps) || !float_eq(v_ortho.clip_pos.y, -0.5f, eps)) {
        display_print("[PHASE2] ERROR: Ortho projection failed!\n");
        passed = false;
    }

    // 6. Push / Pop Stack Integrity
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(123.0f, 456.0f, 789.0f);
    glPushMatrix();

    glTranslatef(10.0f, 10.0f, 10.0f); // Modify top
    glPopMatrix(); // Restore top

    GLMatrix4x4* M = gl_state_get_current_matrix(state);
    if (!float_eq(M->m[12], 123.0f, eps) || !float_eq(M->m[13], 456.0f, eps) || !float_eq(M->m[14], 789.0f, eps)) {
        display_print("[PHASE2] ERROR: Push/Pop matrix stack restoration failed!\n");
        passed = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (passed) {
        display_print("[PHASE2] Matrix Stack & Transformation Golden Test: PASS\n");
        crash_log_add("[PHASE2 TEST] Matrix Stack & Transformation Golden Test: PASS");
    } else {
        display_print("[PHASE2] Matrix Stack & Transformation Golden Test: FAIL!\n");
        crash_log_add("[PHASE2 TEST] Matrix Stack & Transformation Golden Test: FAIL");
    }
}

static void test_gl_multi_context_isolation(uint32_t win1, uint32_t win2) {
    display_print("\n[PHASE2] Test C: Multi-Context GL State Isolation...\n");

    BGLDrawable* d1 = bglCreateDrawableForWindow(win1);
    BGLDrawable* d2 = bglCreateDrawableForWindow(win2);
    BGLContext* c1 = bglCreateContext(d1);
    BGLContext* c2 = bglCreateContext(d2);

    if (!d1 || !d2 || !c1 || !c2) {
        display_print("[PHASE2] ERROR: Failed to create BGL contexts for isolation test!\n");
        return;
    }

    // Context 1 Setup: Red Clear, ModelView Matrix (tx=100), Green Current Color
    bglMakeCurrent(c1, d1);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(100.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);

    // Context 2 Setup: Blue Clear, Projection Matrix (ty=200), Yellow Current Color
    bglMakeCurrent(c2, d2);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glTranslatef(0.0f, 200.0f, 0.0f);
    glColor3f(1.0f, 1.0f, 0.0f);

    bool isolated = true;

    // Verify Context 1 State upon re-binding
    bglMakeCurrent(c1, d1);
    GLContextState* s1 = gl_state_get_current();
    if (!s1 || s1->clear_color[0] != 1.0f || s1->clear_color[2] != 0.0f) {
        isolated = false;
        display_print("[PHASE2] ERROR: Context 1 clear color leaked!\n");
    }
    if (s1->matrix_mode != GL_MODELVIEW || !float_eq(s1->modelview_stack[0].m[12], 100.0f, 1e-3f)) {
        isolated = false;
        display_print("[PHASE2] ERROR: Context 1 matrix state leaked!\n");
    }
    if (s1->current_color[0] != 0.0f || s1->current_color[1] != 1.0f) {
        isolated = false;
        display_print("[PHASE2] ERROR: Context 1 current color leaked!\n");
    }

    // Verify Context 2 State upon re-binding
    bglMakeCurrent(c2, d2);
    GLContextState* s2 = gl_state_get_current();
    if (!s2 || s2->clear_color[0] != 0.0f || s2->clear_color[2] != 1.0f) {
        isolated = false;
        display_print("[PHASE2] ERROR: Context 2 clear color leaked!\n");
    }
    if (s2->matrix_mode != GL_PROJECTION || !float_eq(s2->projection_stack[0].m[13], 200.0f, 1e-3f)) {
        isolated = false;
        display_print("[PHASE2] ERROR: Context 2 matrix state leaked!\n");
    }
    if (s2->current_color[0] != 1.0f || s2->current_color[1] != 1.0f) {
        isolated = false;
        display_print("[PHASE2] ERROR: Context 2 current color leaked!\n");
    }

    bglReleaseCurrent();
    bglDestroyContext(c1);
    bglDestroyContext(c2);
    bglDestroyDrawable(d1);
    bglDestroyDrawable(d2);

    if (isolated) {
        display_print("[PHASE2] Multi-Context GL State Isolation: PASS\n");
        crash_log_add("[PHASE2 TEST] Multi-Context GL State Isolation: PASS");
    } else {
        display_print("[PHASE2] Multi-Context GL State Isolation: FAIL!\n");
        crash_log_add("[PHASE2 TEST] Multi-Context GL State Isolation: FAIL");
    }
}

static void test_gl_immediate_mode_assembly(uint32_t win_id) {
    display_print("\n[PHASE2] Test D: Immediate Mode & Primitive Assembly Test...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) {
        display_print("[PHASE2] ERROR: Failed to create objects for assembly test!\n");
        return;
    }

    bglMakeCurrent(ctx, d);
    GLContextState* state = gl_state_get_current();

    // 1. Submit Triangle
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -5.0f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -5.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f( 0.0f,  1.0f, -5.0f);
    glEnd();

    bool assembled_ok = true;
    if (state->primitive_count != 1 || state->primitive_buffer[0].type != GL_TRIANGLES) {
        display_print("[PHASE2] ERROR: Triangle assembly count mismatch!\n");
        assembled_ok = false;
    }

    // 2. Submit Quad (should assemble into 2 Triangles)
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f( 1.0f, -1.0f);
    glVertex2f( 1.0f,  1.0f);
    glVertex2f(-1.0f,  1.0f);
    glEnd();

    if (state->primitive_count != 2 || state->primitive_buffer[0].type != GL_TRIANGLES || state->primitive_buffer[1].type != GL_TRIANGLES) {
        display_print("[PHASE2] ERROR: Quad assembly into 2 triangles failed!\n");
        assembled_ok = false;
    }

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (assembled_ok) {
        display_print("[PHASE2] Immediate Mode & Primitive Assembly Test: PASS\n");
        crash_log_add("[PHASE2 TEST] Immediate Mode & Primitive Assembly Test: PASS");
    } else {
        display_print("[PHASE2] Immediate Mode & Primitive Assembly Test: FAIL!\n");
        crash_log_add("[PHASE2 TEST] Immediate Mode & Primitive Assembly Test: FAIL");
    }
}

static void test_gl_visual_clear_presentation(uint32_t win_id) {
    display_print("\n[PHASE2] Test E: Visual glClearColor + glClear Presentation...\n");

    BGLDrawable* d = bglCreateDrawableForWindow(win_id);
    BGLContext* ctx = bglCreateContext(d);
    if (!d || !ctx) {
        display_print("[PHASE2] ERROR: Failed to create objects for visual clear test!\n");
        return;
    }

    bglMakeCurrent(ctx, d);

    // Set OpenGL Clear Color to Cornflower Blue (R=0.39, G=0.58, B=0.93, A=1.0)
    glClearColor(0.392f, 0.584f, 0.929f, 1.0f);

    // Clear Color Buffer via GL API
    glClear(GL_COLOR_BUFFER_BIT);

    // Verify first pixel in color buffer contains Cornflower Blue (0xFF6495ED)
    uint32_t pixel = d->color_buffer[0];
    bool clear_ok = true;

    // Allow minor rounding difference (+-2)
    uint32_t r = (pixel >> 16) & 0xFF;
    uint32_t g = (pixel >> 8) & 0xFF;
    uint32_t b = pixel & 0xFF;

    if (r < 98 || r > 102 || g < 147 || g > 151 || b < 235 || b > 239) {
        display_print("[PHASE2] ERROR: glClear color buffer pixel mismatch! Got 0x");
        display_print_hex(pixel); display_print("\n");
        clear_ok = false;
    }

    // Submit frame through BGL -> BWE -> Compositor -> AGDTE
    bglSwapBuffers(ctx);

    bglReleaseCurrent();
    bglDestroyContext(ctx);
    bglDestroyDrawable(d);

    if (clear_ok) {
        display_print("[PHASE2] Visual glClear Presentation Test: PASS (GL -> BGL -> BWE -> AGDTE)\n");
        crash_log_add("[PHASE2 TEST] Visual glClear Presentation Test: PASS");
    } else {
        display_print("[PHASE2] Visual glClear Presentation Test: FAIL!\n");
        crash_log_add("[PHASE2 TEST] Visual glClear Presentation Test: FAIL");
    }
}

void run_phase2_gl_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 2 VERIFICATION\n");
    display_print("=========================================\n");

    uint32_t win1 = 0, win2 = 0;
    bwe_error_t err1 = BOS_CreateWindow(100, 100, 160, 120, "GL_Window1", &win1);
    bwe_error_t err2 = BOS_CreateWindow(200, 200, 160, 120, "GL_Window2", &win2);

    if (err1 != BWE_SUCCESS || err2 != BWE_SUCCESS || win1 == 0 || win2 == 0) {
        display_print("[PHASE2] ERROR: Failed to create BWE windows for GL testing!\n");
        return;
    }

    test_gl_error_machine(win1);
    test_gl_matrix_transformations(win1);
    test_gl_multi_context_isolation(win1, win2);
    test_gl_immediate_mode_assembly(win1);
    test_gl_visual_clear_presentation(win1);

    BOS_DestroySurface(win1);
    BOS_DestroySurface(win2);

    display_print("=========================================\n");
}
