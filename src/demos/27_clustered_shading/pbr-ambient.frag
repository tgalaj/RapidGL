#version 460 core
#include "../../src/demos/27_clustered_shading/pbr-lighting.glh"

void main()
{
    
    vec3 ambient = indirectLightingDiffuse(normalize(in_normal), in_world_pos);
    frag_color = vec4(ambient, 1.0);
} 