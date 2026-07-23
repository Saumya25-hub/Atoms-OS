#ifndef ATOMS_OS_GL_LIGHTING_H
#define ATOMS_OS_GL_LIGHTING_H

#include "kernel/graphics/gl/gl_types.h"
#include "kernel/graphics/gl/gl_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float emission[4];
    float shininess;
} GLMaterialState;

typedef struct {
    bool  enabled;
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float position[4];       // Stored in Eye-Space (captured at glLightfv time)
    float spot_direction[3]; // Stored in Eye-Space
    float spot_exponent;
    float spot_cutoff;
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;
} GLLightState;

struct GLContextState; // Forward declaration

void gl_lighting_init_material(GLMaterialState* mat);
void gl_lighting_init_light(GLLightState* light, GLuint index);
void gl_lighting_evaluate_vertex(struct GLContextState* state, const float eye_pos[4], const float eye_normal[3], const float current_color[4], float out_color[4]);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_GL_LIGHTING_H
