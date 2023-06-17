#version 460 core
#include "lighting.glh"

uniform SpotLight spot_light;

void main()
{
    frag_color = reinhard(calcSpotLight(spot_light, normalize(normal), world_pos));
} 