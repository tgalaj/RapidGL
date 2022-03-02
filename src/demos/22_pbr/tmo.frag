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

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
mat3 ACESInputMat =
{
    {0.59719, 0.07600, 0.02840},
    {0.35458, 0.90834, 0.13383},
    {0.04823, 0.01566, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
mat3 ACESOutputMat =
{
    { 1.60475, -0.10208, -0.00327},
    {-0.53108,  1.10813, -0.07276},
    {-0.07367, -0.00605,  1.07602 }
};

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

void main()
{
	vec4 x = u_exposure * texture(u_filter_texture, texcoord);
	    
    vec3 color = ACESInputMat * x.rgb;
         color = RRTAndODTFit(color);
         color = ACESOutputMat * color;

         color = gammaCorrect(color);
         color = clamp(color, 0.0, 1.0);

	frag_color = vec4(color, 1.0);
}