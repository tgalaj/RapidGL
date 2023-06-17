#version 460 core
#include "lighting-terrain.glh"

uniform float ambient_factor;

void main()
{
    frag_color = reinhard(blendedTerrainColor(normalize(normal)) * vec4(vec3(ambient_factor), 1.0));
} 