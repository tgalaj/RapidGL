#version 460 core
#include "lighting-terrain.glh"

uniform DirectionalLight directional_light;

void main()
{
    frag_color = reinhard(calcDirectionalLight(directional_light, normalize(normal), world_pos));
} 