#version 460 core
#include "../../src/demos/22_pbr/pbr-lighting.glh"

layout (location = 3) in vec4 in_pos_light_space;

uniform DirectionalLight u_directional_light;

layout (binding = 8) uniform sampler2DShadow u_shadow_map;

float calcShadow(vec4 pos_light_space)
{
    vec4 shadow_coord = in_pos_light_space * 0.5 + 0.5;

    if (shadow_coord.z > 1.0) return 0.0;

    float bias = max(0.0001 * (1.0 - dot(normalize(in_normal), -u_directional_light.direction)), 0.00001);  
    shadow_coord.z -= bias;
    
    float sum = 0;

    for(int i = -1; i <= 1; ++i)
    {
        for(int j = -1; j <= 1; ++j)
        {
            sum += textureProjOffset(u_shadow_map, shadow_coord, ivec2(i, j));
        }
    }

    return sum / 9.0;
}

void main()
{
    float shadow = calcShadow(in_pos_light_space);
    frag_color = vec4(shadow * calcDirectionalLight(u_directional_light, normalize(in_normal), in_world_pos), 1.0);
} 