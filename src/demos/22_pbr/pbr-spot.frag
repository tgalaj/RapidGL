#version 460 core
#include "../../src/demos/22_pbr/pbr-lighting.glh"

uniform SpotLight u_spot_light;

void main()
{
    frag_color = vec4(calcSpotLight(u_spot_light, normalize(in_normal), in_world_pos), 1.0);
} 