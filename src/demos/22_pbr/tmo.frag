#version 450

// ACES by K. Narkowicz

in vec2 texcoord;
out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_filter_texture;

uniform float u_exposure;
uniform float u_gamma;

vec4 gammaCorrect(vec4 color) 
{
    return pow(color, vec4(1.0/u_gamma));
}

void main()
{
	vec4 x = u_exposure * texture(u_filter_texture, texcoord);
	
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;

	vec4 color = (x*(a*x + b)) / (x*(c*x + d) + e);

	color.a = 1.0;
	color = clamp(color, 0.0, 1.0);

	frag_color = vec4(gammaCorrect(color).rgb, 1.0);
}