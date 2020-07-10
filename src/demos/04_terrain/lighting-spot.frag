#version 460 core
#include "../../src/demos/03_lighting/lighting.glh"

uniform SpotLight spot_light;

void main()
{
    frag_color = calcSpotLight(spot_light, normalize(normal), world_pos);
} 