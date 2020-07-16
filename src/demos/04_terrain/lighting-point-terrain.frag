#version 460 core
#include "../../src/demos/04_terrain/lighting-terrain.glh"

uniform PointLight point_light;

void main()
{
    frag_color = calcPointLight(point_light, normalize(normal), world_pos);
} 