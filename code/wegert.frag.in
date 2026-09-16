#version 300 es
precision highp float;
precision highp int;

/*__WEGERT_COLOR_CORE__*/

#define MAX_FACTORS 64

in vec2 v_ndc;
out vec4 frag_color;

uniform vec2 u_center;
uniform float u_half_height;
uniform float u_aspect;
uniform vec2 u_resolution;
uniform int u_zero_count;
uniform int u_pole_count;
uniform vec2 u_zeros[MAX_FACTORS];
uniform vec2 u_poles[MAX_FACTORS];

void main() {
    vec2 z = u_center + vec2(
        v_ndc.x * u_half_height * u_aspect,
        v_ndc.y * u_half_height
    );

    float phase = 0.0;
    float log_modulus = 0.0;

    for (int index = 0; index < MAX_FACTORS; ++index) {
        if (index >= u_zero_count) {
            break;
        }
        vec2 delta = z - u_zeros[index];
        phase += atan(delta.y, delta.x);
        log_modulus += log(max(length(delta), 1.0e-12));
    }

    for (int index = 0; index < MAX_FACTORS; ++index) {
        if (index >= u_pole_count) {
            break;
        }
        vec2 delta = z - u_poles[index];
        phase -= atan(delta.y, delta.x);
        log_modulus -= log(max(length(delta), 1.0e-12));
    }

    vec3 color = wegert_color_from_phase_log_modulus(phase, log_modulus);

    // Keep factors visible without replacing the phase portrait with UI chrome.
    float world_per_pixel = (2.0 * u_half_height) / max(u_resolution.y, 1.0);
    vec3 marker_dark = vec3(0.09411765);
    vec3 marker_light = vec3(0.96078431, 0.94901961, 0.92156863);

    for (int index = 0; index < MAX_FACTORS; ++index) {
        if (index >= u_zero_count) {
            break;
        }
        float radius = length(z - u_zeros[index]) / world_per_pixel;
        if (radius < 6.0) {
            color = radius < 4.2 ? marker_light : marker_dark;
        }
    }

    for (int index = 0; index < MAX_FACTORS; ++index) {
        if (index >= u_pole_count) {
            break;
        }
        vec2 offset = (z - u_poles[index]) / world_per_pixel;
        float extent = max(abs(offset.x), abs(offset.y));
        float diagonal = abs(abs(offset.x) - abs(offset.y));
        if (extent < 6.0 && diagonal < 1.15) {
            color = marker_dark;
        }
    }

    frag_color = vec4(color, 1.0);
}
