#version 460 core
#include "pbr_lighting.glh"

out vec4 frag_color;

uniform float u_near_z;
uniform float u_far_z;
uniform float u_slice_scale;
uniform float u_slice_bias;
uniform bool  u_debug_slices;
uniform vec2  u_tile_size_in_px;
uniform uvec3 u_grid_size;

layout (binding = 9) uniform sampler2D u_depth_buffer;

const vec3 debug_colors[8] = vec3[]
(
   vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
   vec3(1, 0, 0), vec3(1, 0, 1), vec3(1, 1, 0), vec3(1, 1, 1)
);

layout(std430, binding = DIRECTIONAL_LIGHTS_SSBO_BINDING_INDEX) buffer DirLightsSSBO
{
	DirectionalLight dir_lights[];
};

layout(std430, binding = POINT_LIGHTS_SSBO_BINDING_INDEX) buffer PointLightSSBO
{
	PointLight point_lights[];
};

layout(std430, binding = SPOT_LIGHTS_SSBO_BINDING_INDEX) buffer SpotLightsSSBO
{
	SpotLight spot_lights[];
};

layout (std430, binding = LIGHT_INDEX_LIST_SSBO_BINDING_INDEX) buffer LightIndexListSSBO
{
    uint global_light_index_list[];
};

layout (std430, binding = LIGHT_GRID_SSBO_BINDING_INDEX) buffer LightGridSSBO
{
	uint global_index_count;
    LightGrid light_grid[];
};

void main()
{
	vec3 radiance = vec3(0.0);
	vec3 normal = normalize(in_normal);

	MaterialProperties material = getMaterialProperties(normal);

	// Locating the cluster we are in
	// TODO: load depth from depth buffer instead of gl_fragcoord.z
	float frag_depth = texture(u_depth_buffer, gl_FragCoord.xy / vec2(1920, 1080)).r;
	//float frag_depth = gl_FragCoord.z;
	uint  tile_z     = uint(max(log2(linearDepth(frag_depth, u_near_z, u_far_z)) * u_slice_scale + u_slice_bias, 0.0));
	uvec3 tiles      = uvec3(uvec2(gl_FragCoord.xy * u_tile_size_in_px), tile_z);
	uint  tile_index = tiles.x + u_grid_size.x * tiles.y + (u_grid_size.x * u_grid_size.y) * tiles.z;

	// Calculate the directional lights
	for (uint i = 0; i < dir_lights.length(); ++i)
	{
		radiance += calcDirectionalLight(dir_lights[i], in_world_pos, material);
	}

	// Calculate the point lights
	uint point_light_count		  = light_grid[tile_index].count;
	uint point_light_index_offset = light_grid[tile_index].offset;

	#if 1
	for (uint i = 0; i < point_light_count; ++i)
	{
		uint light_index = global_light_index_list[point_light_index_offset + i];
		radiance += calcPointLight(point_lights[light_index], in_world_pos, material);
	}
	#else
	for (uint i = 0; i < point_lights.length(); ++i)
	{
		radiance += calcPointLight(point_lights[i], in_world_pos, material);
	}
	#endif
	// Calculate the spot lights
	for (uint i = 0; i < spot_lights.length(); ++i)
	{
		radiance += calcSpotLight(spot_lights[i], in_world_pos, material);
	}

	radiance += indirectLightingIBL(in_world_pos, material);
	radiance += material.emission;

	if (u_debug_slices)
	{
		frag_color = vec4(debug_colors[tile_z % 8], 1.0);
	}
	else
	{
		// total lighting
		frag_color = vec4(radiance, 1.0);
	}
}