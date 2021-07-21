#version 450

// AMD (Lottes) TMO
// Taken from: https://github.com/tizian/tonemapper/blob/master/src/operators/amd.h 

in vec2 texcoord;
out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_filter_texture;

uniform float u_exposure;
uniform float u_gamma;
uniform float u_a;
uniform float u_d;
uniform float u_hdr_max;
uniform float u_mid_in;
uniform float u_mid_out;

vec4 gammaCorrect(vec4 color) 
{
    return pow(color, vec4(1.0/u_gamma));
}

float map(float x) 
{
    float b = (-pow(u_mid_in, u_a) + pow(u_hdr_max, u_a) * u_mid_out) /
              ((pow(u_hdr_max, u_a * u_d) - pow(u_mid_in, u_a * u_d)) * u_mid_out);
    float c = (pow(u_hdr_max, u_a * u_d) * pow(u_mid_in, u_a) - pow(u_hdr_max, u_a) * pow(u_mid_in, u_a * u_d) * u_mid_out) /
              ((pow(u_hdr_max, u_a * u_d) - pow(u_mid_in, u_a * u_d)) * u_mid_out);
    return pow(x, u_a) / (pow(x, u_a * u_d) * b + c);
}

void main()
{
	vec4 color = u_exposure * texture(u_filter_texture, texcoord);
	color = vec4(map(color.r), map(color.g), map(color.b), color.a);
	color.a = 1.0;
	color = clamp(color, 0.0, 1.0);
	frag_color = vec4(gammaCorrect(color).rgb, 1.0);
}