#version 460 core
#include "../../src/demos/04_terrain/lighting-terrain.glh"

uniform float ambient_factor;

void main()
{
    frag_color = reinhard(blendedTerrainColor(normalize(normal)) * vec4(vec3(ambient_factor), 1.0));
} 