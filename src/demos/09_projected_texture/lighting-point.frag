#version 460 core
#include "../../src/demos/03_lighting/lighting.glh"

uniform PointLight point_light;

void main()
{
    frag_color = reinhard(calcPointLight(point_light, normalize(normal), world_pos));
} 