#version 460 core
#include "pbr-lighting.glh"

uniform PointLight u_point_light;

void main()
{
    frag_color = vec4(calcPointLight(u_point_light, normalize(in_normal), in_world_pos), 1.0);
} 