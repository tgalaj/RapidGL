#version 450

// Soruce: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

in vec2 texcoord;
out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_filter_texture;

uniform float u_exposure;
uniform float u_gamma;

vec3 gammaCorrect(vec3 color) 
{
    return pow(color, vec3(1.0/u_gamma));
}

vec3 Tonemap_Filmic_UC2(vec3 linearColor, float linearWhite, float A, float B, float C, float D, float E, float F) 
{
	// Uncharted II configurable tonemapper.

	// A = shoulder strength
	// B = linear strength
	// C = linear angle
	// D = toe strength
	// E = toe numerator
	// F = toe denominator
	// Note: E / F = toe angle
	// linearWhite = linear white point value

	vec3 x = linearColor;
	vec3 color = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
	
	x = vec3(linearWhite);
	vec3 white = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
	
	return color / white;
}

vec3 Tonemap_Filmic_UC2Default(vec3 linearColor) {

	// Uncharted II fixed tonemapping formula.
	// Gives a warm and gritty image, saturated shadows and bleached highlights.

	return Tonemap_Filmic_UC2(linearColor, 11.2, 0.22, 0.3, 0.1, 0.2, 0.01, 0.30);
}

void main()
{
	vec4 x = u_exposure * texture(u_filter_texture, texcoord);
	    
    vec3 color = Tonemap_Filmic_UC2Default(x.rgb);
		 color = gammaCorrect(color);

	frag_color = vec4(color, 1.0);
}