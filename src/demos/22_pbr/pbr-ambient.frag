#version 460 core
#include "../../src/demos/22_pbr/pbr-lighting.glh"

void main()
{
    vec3 ambient = vec3(0.03) * u_albedo * u_ao;
    frag_color = vec4(ambient, 1.0);
} 