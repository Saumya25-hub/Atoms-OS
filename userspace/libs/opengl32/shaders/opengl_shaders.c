#include "../include/opengl32_api.h"

static GLuint g_shader_counter = 1;
static GLuint g_program_counter = 1;

GLuint glCreateShader(GLenum type) {
    (void)type;
    return g_shader_counter++;
}

void glCompileShader(GLuint shader) {
    (void)shader;
}

GLuint glCreateProgram(void) {
    return g_program_counter++;
}

void glAttachShader(GLuint program, GLuint shader) {
    (void)program; (void)shader;
}

void glLinkProgram(GLuint program) {
    (void)program;
}

void glUseProgram(GLuint program) {
    (void)program;
}
