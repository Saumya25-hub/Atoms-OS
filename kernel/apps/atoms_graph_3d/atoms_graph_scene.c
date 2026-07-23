#include "atoms_graph_scene.h"
#include "kernel/graphics/gl/gl.h"
#include "kernel/graphics/gl/gl_math.h"
#include "kernel/core/memory/heap/include/heap.h"

// Helper to draw a colored 3D cube (12 triangles, 36 vertices)
static void draw_cube(float size) {
    float h = size * 0.5f;

    glBegin(GL_TRIANGLES);
    // Front face (Red/Yellow)
    glColor3f(1.0f, 0.2f, 0.2f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-h, -h,  h);
    glColor3f(1.0f, 0.8f, 0.2f); glTexCoord2f(1.0f, 0.0f); glVertex3f( h, -h,  h);
    glColor3f(1.0f, 1.0f, 0.2f); glTexCoord2f(1.0f, 1.0f); glVertex3f( h,  h,  h);

    glColor3f(1.0f, 0.2f, 0.2f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-h, -h,  h);
    glColor3f(1.0f, 1.0f, 0.2f); glTexCoord2f(1.0f, 1.0f); glVertex3f( h,  h,  h);
    glColor3f(0.8f, 0.2f, 0.2f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-h,  h,  h);

    // Back face (Cyan/Blue)
    glColor3f(0.2f, 0.8f, 1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-h, -h, -h);
    glColor3f(0.2f, 0.4f, 1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-h,  h, -h);
    glColor3f(0.2f, 1.0f, 1.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f( h,  h, -h);

    glColor3f(0.2f, 0.8f, 1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-h, -h, -h);
    glColor3f(0.2f, 1.0f, 1.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f( h,  h, -h);
    glColor3f(0.2f, 0.6f, 0.8f); glTexCoord2f(0.0f, 0.0f); glVertex3f( h, -h, -h);

    // Top face (Green)
    glColor3f(0.2f, 1.0f, 0.4f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-h,  h, -h);
    glColor3f(0.4f, 1.0f, 0.6f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-h,  h,  h);
    glColor3f(0.6f, 1.0f, 0.8f); glTexCoord2f(1.0f, 0.0f); glVertex3f( h,  h,  h);

    glColor3f(0.2f, 1.0f, 0.4f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-h,  h, -h);
    glColor3f(0.6f, 1.0f, 0.8f); glTexCoord2f(1.0f, 0.0f); glVertex3f( h,  h,  h);
    glColor3f(0.4f, 1.0f, 0.4f); glTexCoord2f(1.0f, 1.0f); glVertex3f( h,  h, -h);

    // Bottom face (Magenta)
    glColor3f(1.0f, 0.2f, 1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-h, -h, -h);
    glColor3f(0.8f, 0.2f, 0.8f); glTexCoord2f(0.0f, 1.0f); glVertex3f( h, -h, -h);
    glColor3f(0.6f, 0.2f, 0.6f); glTexCoord2f(0.0f, 0.0f); glVertex3f( h, -h,  h);

    glColor3f(1.0f, 0.2f, 1.0f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-h, -h, -h);
    glColor3f(0.6f, 0.2f, 0.6f); glTexCoord2f(0.0f, 0.0f); glVertex3f( h, -h,  h);
    glColor3f(1.0f, 0.4f, 1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-h, -h,  h);

    // Right face (Orange)
    glColor3f(1.0f, 0.6f, 0.2f); glTexCoord2f(0.0f, 0.0f); glVertex3f( h, -h, -h);
    glColor3f(1.0f, 0.8f, 0.4f); glTexCoord2f(1.0f, 0.0f); glVertex3f( h,  h, -h);
    glColor3f(1.0f, 0.4f, 0.2f); glTexCoord2f(1.0f, 1.0f); glVertex3f( h,  h,  h);

    glColor3f(1.0f, 0.6f, 0.2f); glTexCoord2f(0.0f, 0.0f); glVertex3f( h, -h, -h);
    glColor3f(1.0f, 0.4f, 0.2f); glTexCoord2f(1.0f, 1.0f); glVertex3f( h,  h,  h);
    glColor3f(1.0f, 0.5f, 0.2f); glTexCoord2f(0.0f, 1.0f); glVertex3f( h, -h,  h);

    // Left face (Purple)
    glColor3f(0.6f, 0.2f, 1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-h, -h, -h);
    glColor3f(0.4f, 0.2f, 0.8f); glTexCoord2f(1.0f, 1.0f); glVertex3f(-h, -h,  h);
    glColor3f(0.8f, 0.4f, 1.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-h,  h,  h);

    glColor3f(0.6f, 0.2f, 1.0f); glTexCoord2f(1.0f, 0.0f); glVertex3f(-h, -h, -h);
    glColor3f(0.8f, 0.4f, 1.0f); glTexCoord2f(0.0f, 1.0f); glVertex3f(-h,  h,  h);
    glColor3f(0.5f, 0.2f, 0.9f); glTexCoord2f(0.0f, 0.0f); glVertex3f(-h,  h, -h);
    glEnd();
}

// Generate procedural 64x64 checkerboard texture
static void generate_checker_texture(GLuint* out_tex) {
    uint32_t pixels[64 * 64];
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            bool check = ((x / 8) + (y / 8)) % 2 == 0;
            if (check) {
                pixels[y * 64 + x] = 0xFF89B4FA; // Soft Cyan
            } else {
                pixels[y * 64 + x] = 0xFFF38BA8; // Soft Pink
            }
        }
    }

    glGenTextures(1, out_tex);
    glBindTexture(GL_TEXTURE_2D, *out_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void atoms_graph_scene_init_gl(AtomsGraphGLResources* res, uint32_t viewport_w, uint32_t viewport_h) {
    if (!res) return;
    
    generate_checker_texture(&res->tex_procedural);
    
    res->fbo_w = 256;
    res->fbo_h = 256;
    
    // Create FBO for Stage 6/7/9 Render-to-Texture
    glGenFramebuffers(1, &res->fbo_offscreen);
    glBindFramebuffer(GL_FRAMEBUFFER, res->fbo_offscreen);
    
    glGenTextures(1, &res->fbo_color_tex);
    glBindTexture(GL_TEXTURE_2D, res->fbo_color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res->fbo_w, res->fbo_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res->fbo_color_tex, 0);
    
    glGenRenderbuffers(1, &res->fbo_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, res->fbo_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, res->fbo_w, res->fbo_h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, res->fbo_depth_rbo);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    res->gl_resources_init = true;
}

void atoms_graph_scene_cleanup_gl(AtomsGraphGLResources* res) {
    if (!res || !res->gl_resources_init) return;
    
    if (res->tex_procedural) {
        glDeleteTextures(1, &res->tex_procedural);
        res->tex_procedural = 0;
    }
    if (res->fbo_color_tex) {
        glDeleteTextures(1, &res->fbo_color_tex);
        res->fbo_color_tex = 0;
    }
    if (res->fbo_depth_rbo) {
        glDeleteRenderbuffers(1, &res->fbo_depth_rbo);
        res->fbo_depth_rbo = 0;
    }
    if (res->fbo_offscreen) {
        glDeleteFramebuffers(1, &res->fbo_offscreen);
        res->fbo_offscreen = 0;
    }
    res->gl_resources_init = false;
}

static void set_perspective(float aspect) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float f = 1.0f; // top/bottom frustum bounds
    glFrustum(-f * aspect, f * aspect, -f, f, 1.0f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void atoms_graph_scene_render_stage(uint32_t stage_idx, float angle_deg, AtomsGraphGLResources* res,
                                     uint32_t viewport_w, uint32_t viewport_h,
                                     uint32_t* out_triangles, uint32_t* out_draw_calls) {
    if (viewport_w == 0 || viewport_h == 0) return;
    float aspect = (float)viewport_w / (float)viewport_h;

    uint32_t tris = 0;
    uint32_t calls = 0;

    glViewport(0, 0, viewport_w, viewport_h);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_TEXTURE_2D);

    switch (stage_idx) {
        case 0: { // STAGE 1: BASELINE GEOMETRY
            glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);
            glTranslatef(0.0f, 0.0f, -4.0f);
            glRotatef(angle_deg, 1.0f, 1.0f, 0.0f);
            draw_cube(1.5f);
            tris = 12;
            calls = 1;
            break;
        }
        case 1: { // STAGE 2: GEOMETRY SCALING (Sphere grid mesh)
            glClearColor(0.09f, 0.10f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);
            glTranslatef(0.0f, 0.0f, -4.5f);
            glRotatef(angle_deg * 0.7f, 0.0f, 1.0f, 0.5f);

            int stacks = 18;
            int slices = 18;
            float r = 1.6f;

            glBegin(GL_TRIANGLES);
            for (int i = 0; i < stacks; i++) {
                float lat0 = GL_PI * (-0.5f + (float)i / stacks);
                float z0  = gl_sinf(lat0) * r;
                float zr0 = gl_cosf(lat0) * r;

                float lat1 = GL_PI * (-0.5f + (float)(i + 1) / stacks);
                float z1  = gl_sinf(lat1) * r;
                float zr1 = gl_cosf(lat1) * r;

                for (int j = 0; j < slices; j++) {
                    float lng0 = 2.0f * GL_PI * (float)j / slices;
                    float x0 = gl_cosf(lng0);
                    float y0 = gl_sinf(lng0);

                    float lng1 = 2.0f * GL_PI * (float)(j + 1) / slices;
                    float x1 = gl_cosf(lng1);
                    float y1 = gl_sinf(lng1);

                    glColor3f(0.2f + 0.8f * (float)i / stacks, 0.5f, 0.8f - 0.5f * (float)j / slices);

                    glVertex3f(x0 * zr0, y0 * zr0, z0);
                    glVertex3f(x1 * zr0, y1 * zr0, z0);
                    glVertex3f(x0 * zr1, y0 * zr1, z1);

                    glVertex3f(x1 * zr0, y1 * zr0, z0);
                    glVertex3f(x1 * zr1, y1 * zr1, z1);
                    glVertex3f(x0 * zr1, y0 * zr1, z1);

                    tris += 2;
                }
            }
            glEnd();
            calls = 1;
            break;
        }
        case 2: { // STAGE 3: DEPTH COMPLEXITY (Overlapping cubes)
            glClearColor(0.12f, 0.08f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            float offsets[5][3] = {
                { 0.0f,  0.0f, -4.0f},
                {-1.2f,  0.8f, -4.8f},
                { 1.2f, -0.6f, -3.8f},
                {-0.8f, -1.0f, -5.2f},
                { 0.9f,  1.1f, -4.2f}
            };

            for (int i = 0; i < 5; i++) {
                glPushMatrix();
                glTranslatef(offsets[i][0], offsets[i][1], offsets[i][2]);
                glRotatef(angle_deg + (i * 45.0f), 0.5f, 1.0f, 0.2f);
                draw_cube(1.2f);
                glPopMatrix();
                tris += 12;
                calls++;
            }
            break;
        }
        case 3: { // STAGE 4: TEXTURE WORKLOAD
            glClearColor(0.08f, 0.14f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, res->tex_procedural);

            glTranslatef(0.0f, 0.0f, -4.0f);
            glRotatef(angle_deg, 1.0f, 0.8f, 0.3f);
            draw_cube(1.6f);

            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            tris = 12;
            calls = 1;
            break;
        }
        case 4: { // STAGE 5: MULTI-OBJECT SCENE (16 Grid Cubes)
            glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            for (int row = -2; row <= 1; row++) {
                for (int col = -2; col <= 1; col++) {
                    glPushMatrix();
                    glTranslatef((float)col * 1.4f + 0.7f, (float)row * 1.4f + 0.7f, -6.5f);
                    glRotatef(angle_deg * 1.2f + (row + col) * 30.0f, 1.0f, 0.5f, 0.2f);
                    draw_cube(0.7f);
                    glPopMatrix();
                    tris += 12;
                    calls++;
                }
            }
            break;
        }
        case 5: { // STAGE 6: RENDER-TO-TEXTURE (Offscreen pass -> main viewport)
            // Pass 1: Render offscreen scene into FBO
            glBindFramebuffer(GL_FRAMEBUFFER, res->fbo_offscreen);
            glViewport(0, 0, res->fbo_w, res->fbo_h);
            glClearColor(0.2f, 0.1f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(1.0f);
            glTranslatef(0.0f, 0.0f, -3.5f);
            glRotatef(angle_deg * 2.0f, 0.0f, 1.0f, 1.0f);
            draw_cube(1.2f);
            tris += 12;
            calls++;

            // Pass 2: Render offscreen texture mapped on main viewport cube
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, viewport_w, viewport_h);
            glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, res->fbo_color_tex);
            glTranslatef(0.0f, 0.0f, -4.0f);
            glRotatef(angle_deg, 1.0f, 0.6f, 0.0f);
            draw_cube(1.6f);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            tris += 12;
            calls++;
            break;
        }
        case 6: { // STAGE 7: RTT STRESS (Multi-pass target switching)
            for (int pass = 0; pass < 2; pass++) {
                glBindFramebuffer(GL_FRAMEBUFFER, res->fbo_offscreen);
                glViewport(0, 0, res->fbo_w, res->fbo_h);
                glClearColor(pass * 0.4f, 0.2f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                set_perspective(1.0f);
                glTranslatef(0.0f, 0.0f, -3.0f);
                glRotatef(angle_deg * (pass + 1), 1.0f, 0.0f, 0.0f);
                draw_cube(1.0f);
                tris += 12;
                calls++;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, viewport_w, viewport_h);
            glClearColor(0.05f, 0.12f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, res->fbo_color_tex);

            for (int i = 0; i < 3; i++) {
                glPushMatrix();
                glTranslatef((float)(i - 1) * 1.8f, 0.0f, -4.5f);
                glRotatef(angle_deg + (i * 60.0f), 0.2f, 1.0f, 0.5f);
                draw_cube(1.1f);
                glPopMatrix();
                tris += 12;
                calls++;
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            break;
        }
        case 7: { // STAGE 8: HIGH GEOMETRY STRESS (30x30 terrain mesh = 1800 tris)
            glClearColor(0.07f, 0.07f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            glTranslatef(0.0f, -1.0f, -5.5f);
            glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(angle_deg * 0.5f, 0.0f, 1.0f, 0.0f);

            int grid_dim = 30;
            float step = 4.0f / (float)grid_dim;

            glBegin(GL_TRIANGLES);
            for (int z = 0; z < grid_dim - 1; z++) {
                float z0 = -2.0f + (float)z * step;
                float z1 = z0 + step;

                for (int x = 0; x < grid_dim - 1; x++) {
                    float x0 = -2.0f + (float)x * step;
                    float x1 = x0 + step;

                    float y00 = gl_sinf((x0 + z0) * 3.0f + angle_deg * 0.05f) * 0.3f;
                    float y10 = gl_sinf((x1 + z0) * 3.0f + angle_deg * 0.05f) * 0.3f;
                    float y01 = gl_sinf((x0 + z1) * 3.0f + angle_deg * 0.05f) * 0.3f;
                    float y11 = gl_sinf((x1 + z1) * 3.0f + angle_deg * 0.05f) * 0.3f;

                    glColor3f(0.2f + (x0 + 2.0f) * 0.2f, 0.4f + y00 * 0.5f, 0.8f);
                    glVertex3f(x0, y00, z0);
                    glVertex3f(x1, y10, z0);
                    glVertex3f(x0, y01, z1);

                    glVertex3f(x1, y10, z0);
                    glVertex3f(x1, y11, z1);
                    glVertex3f(x0, y01, z1);

                    tris += 2;
                }
            }
            glEnd();
            calls = 1;
            break;
        }
        case 8: // STAGE 9: COMBINED PIPELINE STRESS
        case 9: { // STAGE 10: SUSTAINED STABILITY RUN
            // Offscreen pass
            glBindFramebuffer(GL_FRAMEBUFFER, res->fbo_offscreen);
            glViewport(0, 0, res->fbo_w, res->fbo_h);
            glClearColor(0.3f, 0.1f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(1.0f);
            glTranslatef(0.0f, 0.0f, -3.0f);
            glRotatef(angle_deg * 2.5f, 1.0f, 1.0f, 0.0f);
            draw_cube(1.1f);
            tris += 12;
            calls++;

            // Main pass
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, viewport_w, viewport_h);
            glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            set_perspective(aspect);

            // Background terrain mesh
            glPushMatrix();
            glTranslatef(0.0f, -1.2f, -6.0f);
            glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(angle_deg * 0.3f, 0.0f, 1.0f, 0.0f);

            int grid_dim = 24;
            float step = 4.0f / (float)grid_dim;

            glBegin(GL_TRIANGLES);
            for (int z = 0; z < grid_dim - 1; z++) {
                float z0 = -2.0f + (float)z * step;
                float z1 = z0 + step;
                for (int x = 0; x < grid_dim - 1; x++) {
                    float x0 = -2.0f + (float)x * step;
                    float x1 = x0 + step;

                    float y00 = gl_sinf((x0 + z0) * 2.5f + angle_deg * 0.05f) * 0.25f;
                    float y10 = gl_sinf((x1 + z0) * 2.5f + angle_deg * 0.05f) * 0.25f;
                    float y01 = gl_sinf((x0 + z1) * 2.5f + angle_deg * 0.05f) * 0.25f;
                    float y11 = gl_sinf((x1 + z1) * 2.5f + angle_deg * 0.05f) * 0.25f;

                    glColor3f(0.1f + (x0 + 2.0f) * 0.2f, 0.3f, 0.6f + y00);
                    glVertex3f(x0, y00, z0);
                    glVertex3f(x1, y10, z0);
                    glVertex3f(x0, y01, z1);

                    glVertex3f(x1, y10, z0);
                    glVertex3f(x1, y11, z1);
                    glVertex3f(x0, y01, z1);

                    tris += 2;
                }
            }
            glEnd();
            calls++;
            glPopMatrix();

            // Foreground textured cube sampling RTT
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, res->fbo_color_tex);
            glPushMatrix();
            glTranslatef(0.0f, 0.5f, -3.8f);
            glRotatef(angle_deg * 1.5f, 0.8f, 1.0f, 0.2f);
            draw_cube(1.3f);
            glPopMatrix();
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);

            tris += 12;
            calls++;
            break;
        }
        default:
            break;
    }

    if (out_triangles) *out_triangles = tris;
    if (out_draw_calls) *out_draw_calls = calls;
}
