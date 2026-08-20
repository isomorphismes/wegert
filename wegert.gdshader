shader_type canvas_item;
render_mode unshaded;

const int MAX_POINTS = 16;
const float PI = 3.14159265358979323846;

uniform int zero_count = 0;
uniform int pole_count = 0;
uniform vec2 zeros[16];
uniform vec2 poles[16];
uniform float phase_offset = 0.0;
uniform float aspect = 1.0;
uniform float y_half_extent = 2.5;

float linear_to_srgb(float x) {
	x = max(x, 0.0);
	if (x <= 0.0031308) {
		return 12.92 * x;
	}
	return 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

vec3 hcl_to_srgb(float hue_degrees, float chroma, float lightness) {
	// R's hcl(): polar CIELUV, D65 white point.
	float h = hue_degrees * PI / 180.0;
	float u_star = chroma * cos(h);
	float v_star = chroma * sin(h);

	const float u_n = 0.19783982482140777;
	const float v_n = 0.4683363029324097;

	if (lightness <= 0.0) {
		return vec3(0.0);
	}

	float u_prime = u_star / (13.0 * lightness) + u_n;
	float v_prime = v_star / (13.0 * lightness) + v_n;
	v_prime = max(v_prime, 0.000001);

	float Y;
	if (lightness > 8.0) {
		Y = pow((lightness + 16.0) / 116.0, 3.0);
	} else {
		Y = lightness / 903.2962962962963;
	}

	float X = 9.0 * Y * u_prime / (4.0 * v_prime);
	float Z = Y * (12.0 - 3.0 * u_prime - 20.0 * v_prime) / (4.0 * v_prime);

	vec3 linear_rgb = vec3(
		 3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z,
		-0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z,
		 0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z
	);

	return clamp(vec3(
		linear_to_srgb(linear_rgb.r),
		linear_to_srgb(linear_rgb.g),
		linear_to_srgb(linear_rgb.b)
	), 0.0, 1.0);
}

void fragment() {
	vec2 z = vec2(
		(UV.x * 2.0 - 1.0) * y_half_extent * aspect,
		(1.0 - UV.y * 2.0) * y_half_extent
	);

	float phase = phase_offset;
	float log_modulus = 0.0;

	for (int index = 0; index < MAX_POINTS; index++) {
		if (index < zero_count) {
			vec2 delta = z - zeros[index];
			float radius = max(length(delta), 0.000001);
			phase += atan(delta.y, delta.x);
			log_modulus += log(radius);
		}
		if (index < pole_count) {
			vec2 delta = z - poles[index];
			float radius = max(length(delta), 0.000001);
			phase -= atan(delta.y, delta.x);
			log_modulus -= log(radius);
		}
	}

	float hue = mod(phase * 180.0 / PI, 360.0);
	if (hue < 0.0) {
		hue += 360.0;
	}

	// Exact scalar structure from the old R Wegert.R palette:
	// l(x) = frac(x / 100), mod(w) = l(|w|),
	// light = 66 + 4*mod(w) + 3*l(hue), chroma = 45.
	float modulus = exp(clamp(log_modulus, -20.0, 20.0));
	float modulus_band = fract(modulus / 100.0);
	float hue_band = fract(hue / 100.0);
	float lightness = 66.0 + 4.0 * modulus_band + 3.0 * hue_band;

	COLOR = vec4(hcl_to_srgb(hue, 45.0, lightness), 1.0);
}
