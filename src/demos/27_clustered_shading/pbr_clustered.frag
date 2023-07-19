#version 460 core
#include "pbr_lighting.glh"

out vec4 frag_color;

uniform float u_near_z;
uniform uvec3 u_grid_dim;
uniform uvec2 u_cluster_size_ss;
uniform float u_log_grid_dim_y;

uniform bool u_debug_slices;
uniform bool u_debug_clusters_occupancy;
uniform float u_debug_clusters_occupancy_blend_factor;

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

layout(std430, binding = AREA_LIGHTS_SSBO_BINDING_INDEX) buffer AreaLightsSSBO
{
    AreaLight area_lights[];
};

layout(std430, binding = POINT_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX) buffer PointLightIndexListSSBO
{
    uint point_light_index_list[];
};

layout(std430, binding = POINT_LIGHT_GRID_SSBO_BINDING_INDEX) buffer PointLightGridSSBO
{
    uint point_light_index_counter;
    LightGrid point_light_grid[];
};

layout(std430, binding = SPOT_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX) buffer SpotLightIndexListSSBO
{
    uint spot_light_index_list[];
};

layout(std430, binding = SPOT_LIGHT_GRID_SSBO_BINDING_INDEX) buffer SpotLightGridSSBO
{
    uint spot_light_index_counter;
    LightGrid spot_light_grid[];
};

layout (std430, binding = AREA_LIGHT_INDEX_LIST_SSBO_BINDING_INDEX) buffer AreaLightIndexListSSBO
{
    uint area_light_index_list[];
};

layout (std430, binding = AREA_LIGHT_GRID_SSBO_BINDING_INDEX) buffer AreaLightGridSSBO
{
    uint area_light_index_counter;
    LightGrid area_light_grid[];
};

uint  computeClusterIndex1D(uvec3 cluster_index3D);
uvec3 computeClusterIndex3D(vec2 screen_pos, float view_z);
vec3  fromRedToGreen(float interpolant);
vec3  fromGreenToBlue(float interpolant);
vec3  heatMap(float interpolant);

void main()
{
    vec3 radiance = vec3(0.0);
    vec3 normal = normalize(in_normal);

    MaterialProperties material = getMaterialProperties(normal);

    // Calculate the directional lights
    for (uint i = 0; i < dir_lights.length(); ++i)
    {
        radiance += calcDirectionalLight(dir_lights[i], in_world_pos, material);
    }

    // Locating the cluster we are in
    uvec3 cluster_index3D = computeClusterIndex3D(gl_FragCoord.xy, in_view_pos.z);
    uint  cluster_index1D = computeClusterIndex1D(cluster_index3D);

    // Calculate the point lights contribution
    uint light_index_offset = point_light_grid[cluster_index1D].offset;
    uint light_count		= point_light_grid[cluster_index1D].count;

    for (uint i = 0; i < light_count; ++i)
    {
        uint light_index = point_light_index_list[light_index_offset + i];
        radiance += calcPointLight(point_lights[light_index], in_world_pos, material);
    }

    // Calculate the spot lights contribution
    light_index_offset = spot_light_grid[cluster_index1D].offset;
    light_count		   = spot_light_grid[cluster_index1D].count;

    for (uint i = 0; i < light_count; ++i)
    {
        uint light_index = spot_light_index_list[light_index_offset + i];
        radiance += calcSpotLight(spot_lights[light_index], in_world_pos, material);
    }

    // Calculate the area lights contribution
    light_index_offset = area_light_grid[cluster_index1D].offset;
    light_count		   = area_light_grid[cluster_index1D].count;

    for (uint i = 0; i < light_count; ++i)
    {
        uint light_index = area_light_index_list[light_index_offset + i];
        radiance += calcLtcAreaLight(area_lights[light_index], in_world_pos, material);
    }

    radiance += indirectLightingIBL(in_world_pos, material);
    radiance += material.emission;

    if (u_debug_slices)
    {
        frag_color = vec4(debug_colors[cluster_index3D.z % 8], 1.0);
    }
    else if (u_debug_clusters_occupancy)
    {
        uint total_light_count = point_light_grid[cluster_index1D].count + spot_light_grid[cluster_index1D].count + area_light_grid[cluster_index1D].count;
        if (total_light_count > 0)
        {
            float normalized_light_count = total_light_count / 100.0;
            vec3 heat_map_color = heatMap(clamp(normalized_light_count, 0.0, 1.0));

            frag_color = vec4(mix(radiance, heat_map_color, u_debug_clusters_occupancy_blend_factor), 1.0);
        }
    }
    else
    {
        // Total lighting
        frag_color = vec4(radiance, 1.0);
    }
}

uint computeClusterIndex1D(uvec3 cluster_index3D)
{
    return cluster_index3D.x + (u_grid_dim.x * (cluster_index3D.y + u_grid_dim.y * cluster_index3D.z));
}

uvec3 computeClusterIndex3D(vec2 screen_pos, float view_z)
{
    uint x = uint(screen_pos.x / u_cluster_size_ss.x);
    uint y = uint(screen_pos.y / u_cluster_size_ss.y);

    // View space z is negative (right-handed coordinate system)
    // so the view-space z coordinate needs to be negated to make it positive.
    uint z = uint(log( -view_z / u_near_z ) * u_log_grid_dim_y);

    return uvec3(x, y, z);
}

// Heat map functions
// source: https://www.shadertoy.com/view/ltlSRj
vec3 fromRedToGreen(float interpolant)
{
    if (interpolant < 0.5)
    {
       return vec3(1.0, 2.0 * interpolant, 0.0); 
    }
    else
    {
        return vec3(2.0 - 2.0 * interpolant, 1.0, 0.0 );
    }
}

vec3 fromGreenToBlue(float interpolant)
{
    if (interpolant < 0.5)
    {
       return vec3(0.0, 1.0, 2.0 * interpolant); 
    }
    else
    {
        return vec3(0.0, 2.0 - 2.0 * interpolant, 1.0 );
    }  
}

vec3 heatMap(float interpolant)
{
    float invertedInterpolant = interpolant;
    if (invertedInterpolant < 0.5)
    {
        float remappedFirstHalf = 1.0 - 2.0 * invertedInterpolant;
        return fromGreenToBlue(remappedFirstHalf);
    }
    else
    {
        float remappedSecondHalf = 2.0 - 2.0 * invertedInterpolant; 
        return fromRedToGreen(remappedSecondHalf);
    }
}