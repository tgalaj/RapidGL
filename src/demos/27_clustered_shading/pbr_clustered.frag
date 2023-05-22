#version 460 core
#include "pbr_lighting.glh"

out vec4 frag_color;

uniform float u_near_z;
uniform float u_far_z;
uniform float u_slice_scale;
uniform float u_slice_bias;
uniform bool  u_debug_slices;

const vec3 debug_colors[8] = vec3[](
   vec3(0.5, 0.2, 0.4), vec3( 0,  0,  1), vec3( 0, 1, 0),  vec3(0, 1,  1),
   vec3(1,  0,  0),     vec3( 1,  0,  1), vec3( 1, 1, 0),  vec3(1, 1, 1)
);

layout(std430, binding = 9) buffer DirLightsSSBO
{
	DirectionalLight dir_lights[];
};

layout(std430, binding = 10) buffer PointLightSSBO
{
	PointLight point_lights[];
};

layout(std430, binding = 11) buffer SpotLightsSSBO
{
	SpotLight spot_lights[];
};

void main()
{
	vec3 radiance = vec3(0.0);
	vec3 normal = normalize(in_normal);

	MaterialProperties material = getMaterialProperties(normal);

	// for debug clustering scheme purposes for now
	uint zTile = uint(max(log2(linearDepth(gl_FragCoord.z, u_near_z, u_far_z)) * u_slice_scale + u_slice_bias, 0.0));

	// Calculate the directional lights
	for (uint i = 0; i < dir_lights.length(); ++i)
	{
		radiance += calcDirectionalLight(dir_lights[i], in_world_pos, material);
	}

	// Calculate the point lights
	for (uint i = 0; i < point_lights.length(); ++i)
	{
		radiance += calcPointLight(point_lights[i], in_world_pos, material);
	}

	// Calculate the spot lights
	for (uint i = 0; i < spot_lights.length(); ++i)
	{
		radiance += calcSpotLight(spot_lights[i], in_world_pos, material);
	}

	radiance += indirectLightingIBL(in_world_pos, material);
	radiance += material.emission;

	if (u_debug_slices)
	{
		frag_color = vec4(debug_colors[zTile % 8], 1.0);
	}
	else
	{
		// total lighting
		frag_color = vec4(radiance, 1.0);
	}
}