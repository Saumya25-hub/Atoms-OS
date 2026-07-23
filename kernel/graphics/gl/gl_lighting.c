#include "kernel/graphics/gl/gl_lighting.h"
#include "kernel/graphics/gl/gl_state.h"
#include "kernel/graphics/gl/gl_math.h"

void gl_lighting_init_material(GLMaterialState* mat) {
    if (!mat) return;
    mat->ambient[0]  = 0.2f; mat->ambient[1]  = 0.2f; mat->ambient[2]  = 0.2f; mat->ambient[3]  = 1.0f;
    mat->diffuse[0]  = 0.8f; mat->diffuse[1]  = 0.8f; mat->diffuse[2]  = 0.8f; mat->diffuse[3]  = 1.0f;
    mat->specular[0] = 0.0f; mat->specular[1] = 0.0f; mat->specular[2] = 0.0f; mat->specular[3] = 1.0f;
    mat->emission[0] = 0.0f; mat->emission[1] = 0.0f; mat->emission[2] = 0.0f; mat->emission[3] = 1.0f;
    mat->shininess   = 0.0f;
}

void gl_lighting_init_light(GLLightState* light, GLuint index) {
    if (!light) return;
    light->enabled = false;

    if (index == 0) {
        light->ambient[0]  = 0.0f; light->ambient[1]  = 0.0f; light->ambient[2]  = 0.0f; light->ambient[3]  = 1.0f;
        light->diffuse[0]  = 1.0f; light->diffuse[1]  = 1.0f; light->diffuse[2]  = 1.0f; light->diffuse[3]  = 1.0f;
        light->specular[0] = 1.0f; light->specular[1] = 1.0f; light->specular[2] = 1.0f; light->specular[3] = 1.0f;
    } else {
        light->ambient[0]  = 0.0f; light->ambient[1]  = 0.0f; light->ambient[2]  = 0.0f; light->ambient[3]  = 1.0f;
        light->diffuse[0]  = 0.0f; light->diffuse[1]  = 0.0f; light->diffuse[2]  = 0.0f; light->diffuse[3]  = 1.0f;
        light->specular[0] = 0.0f; light->specular[1] = 0.0f; light->specular[2] = 0.0f; light->specular[3] = 1.0f;
    }

    light->position[0] = 0.0f; light->position[1] = 0.0f; light->position[2] = 1.0f; light->position[3] = 0.0f;
    light->spot_direction[0] = 0.0f; light->spot_direction[1] = 0.0f; light->spot_direction[2] = -1.0f;
    light->spot_exponent = 0.0f;
    light->spot_cutoff   = 180.0f; // Non-spotlight default

    light->constant_attenuation  = 1.0f;
    light->linear_attenuation    = 0.0f;
    light->quadratic_attenuation = 0.0f;
}

void gl_lighting_evaluate_vertex(GLContextState* state, const float eye_pos[4], const float eye_normal[3], const float current_color[4], float out_color[4]) {
    if (!state || !eye_pos || !eye_normal || !out_color) return;

    if (!state->lighting_enabled) {
        // Lighting disabled -> pass current vertex color
        out_color[0] = current_color[0];
        out_color[1] = current_color[1];
        out_color[2] = current_color[2];
        out_color[3] = current_color[3];
        return;
    }

    GLMaterialState mat = state->front_material;

    // Apply Color Material if enabled
    if (state->color_material_enabled) {
        if (state->color_material_mode == GL_AMBIENT || state->color_material_mode == GL_AMBIENT_AND_DIFFUSE) {
            mat.ambient[0] = current_color[0]; mat.ambient[1] = current_color[1]; mat.ambient[2] = current_color[2]; mat.ambient[3] = current_color[3];
        }
        if (state->color_material_mode == GL_DIFFUSE || state->color_material_mode == GL_AMBIENT_AND_DIFFUSE) {
            mat.diffuse[0] = current_color[0]; mat.diffuse[1] = current_color[1]; mat.diffuse[2] = current_color[2]; mat.diffuse[3] = current_color[3];
        }
        if (state->color_material_mode == GL_SPECULAR) {
            mat.specular[0] = current_color[0]; mat.specular[1] = current_color[1]; mat.specular[2] = current_color[2]; mat.specular[3] = current_color[3];
        }
        if (state->color_material_mode == GL_EMISSION) {
            mat.emission[0] = current_color[0]; mat.emission[1] = current_color[1]; mat.emission[2] = current_color[2]; mat.emission[3] = current_color[3];
        }
    }

    float N[3] = {eye_normal[0], eye_normal[1], eye_normal[2]};
    if (state->normalize_enabled) {
        gl_vec3_normalize(N);
    }

    // Base Color = Emission + GlobalAmbient * MaterialAmbient
    float total_r = mat.emission[0] + state->light_model_ambient[0] * mat.ambient[0];
    float total_g = mat.emission[1] + state->light_model_ambient[1] * mat.ambient[1];
    float total_b = mat.emission[2] + state->light_model_ambient[2] * mat.ambient[2];
    float total_a = mat.diffuse[3]; // Material diffuse alpha

    for (int i = 0; i < GL_MAX_LIGHTS; i++) {
        GLLightState* light = &state->lights[i];
        if (!light->enabled) continue;

        float L[3];
        float atten = 1.0f;

        if (light->position[3] == 0.0f) {
            // Directional Light
            L[0] = light->position[0];
            L[1] = light->position[1];
            L[2] = light->position[2];
            gl_vec3_normalize(L);
        } else {
            // Positional Light
            float dir[3] = {
                light->position[0] - eye_pos[0],
                light->position[1] - eye_pos[1],
                light->position[2] - eye_pos[2]
            };
            float dist = gl_sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
            L[0] = dir[0]; L[1] = dir[1]; L[2] = dir[2];
            gl_vec3_normalize(L);

            float denom = light->constant_attenuation +
                          light->linear_attenuation * dist +
                          light->quadratic_attenuation * (dist * dist);
            if (denom > 1e-7f) {
                atten = 1.0f / denom;
            }

            // Spotlight Evaluation
            if (light->spot_cutoff != 180.0f) {
                float neg_L[3] = {-L[0], -L[1], -L[2]};
                float spot_dot = gl_vec3_dot(neg_L, light->spot_direction);
                float cos_cutoff = gl_cosf(light->spot_cutoff * (GL_PI / 180.0f));

                if (spot_dot < cos_cutoff) {
                    atten = 0.0f;
                } else if (light->spot_exponent > 0.0f) {
                    atten *= gl_powf(spot_dot, light->spot_exponent);
                }
            }
        }

        if (atten <= 0.0f) continue;

        // Ambient Contribution
        float lr = light->ambient[0] * mat.ambient[0];
        float lg = light->ambient[1] * mat.ambient[1];
        float lb = light->ambient[2] * mat.ambient[2];

        // Diffuse Contribution
        float n_dot_l = gl_vec3_dot(N, L);
        if (n_dot_l > 0.0f) {
            lr += n_dot_l * light->diffuse[0] * mat.diffuse[0];
            lg += n_dot_l * light->diffuse[1] * mat.diffuse[1];
            lb += n_dot_l * light->diffuse[2] * mat.diffuse[2];

            // Specular Contribution
            float V[3];
            if (state->light_model_local_viewer) {
                V[0] = -eye_pos[0]; V[1] = -eye_pos[1]; V[2] = -eye_pos[2];
                gl_vec3_normalize(V);
            } else {
                V[0] = 0.0f; V[1] = 0.0f; V[2] = 1.0f; // Infinite viewer along +Z
            }

            // Half-vector H = normalize(L + V)
            float H[3] = {L[0] + V[0], L[1] + V[1], L[2] + V[2]};
            gl_vec3_normalize(H);

            float n_dot_h = gl_vec3_dot(N, H);
            if (n_dot_h > 0.0f && mat.shininess > 0.0f) {
                float spec_factor = gl_powf(n_dot_h, mat.shininess);
                lr += spec_factor * light->specular[0] * mat.specular[0];
                lg += spec_factor * light->specular[1] * mat.specular[1];
                lb += spec_factor * light->specular[2] * mat.specular[2];
            }
        }

        total_r += atten * lr;
        total_g += atten * lg;
        total_b += atten * lb;
    }

    // Clamp final RGBA float color to [0.0, 1.0]
    out_color[0] = (total_r < 0.0f) ? 0.0f : ((total_r > 1.0f) ? 1.0f : total_r);
    out_color[1] = (total_g < 0.0f) ? 0.0f : ((total_g > 1.0f) ? 1.0f : total_g);
    out_color[2] = (total_b < 0.0f) ? 0.0f : ((total_b > 1.0f) ? 1.0f : total_b);
    out_color[3] = (total_a < 0.0f) ? 0.0f : ((total_a > 1.0f) ? 1.0f : total_a);
}
